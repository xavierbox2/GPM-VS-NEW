#include <iostream>
#include <fstream>
#include <string>
#include <string_view>
#include <iterator>
#include <algorithm>
#include <functional>
#include <tuple>
#include <vector>
#include <map>
#include <thread>
#include <future>
#include <cmath>
#include <memory>
#include <filesystem>

#include "Vector3.h"
#include "Range.h"
#include "StructuredGrid.h"
#include "UIParamerers.h"

#include "utils.h"
#include "GpmVsCoupler.h"
#include "gpm_visage_results.h"
#include "gpm_plugin_description.h"
#include "gpm_plugin_helpers.h"

using namespace std;

gpm_visage_link::gpm_visage_link( shared_ptr<IConfiguration>& config, shared_ptr<IMechanicalPropertiesInitializer> props_model )
{
    _config = config;
    _mech_props_model = props_model;

    _config->initialize_vs_options( _visage_options );
    _gpm_vs_name_map["POR"] = "POROSITY"; //this means "POR" in gpm and "POROSITY" in visage.
    _time_step = -1;
    _error = false;

    //stress in X files is in KPa and YM in GPa. We will use MPa for the stress and GPa for YM 
    //cohesion, tensile strength will also be in MPa
    set<string> names = WellKnownVisageNames::ResultsArrayNames::StressTensor( );
    set<string> eff_stress_names = WellKnownVisageNames::ResultsArrayNames::EffectiveStressTensor( );

    for(string name : names) _from_visage_unit_conversion[name] = 0.001f; //reads in KPa, converts to MPa
    for(string name : eff_stress_names) _from_visage_unit_conversion[name] = 0.001f; //reads in KPa, converts to MPa

    _to_visage_unit_conversion =
    {
     {"COHESION", 1000.00f}, //all the program is in MPa ->passed as KPa to vs
     {"TENSILE_STRENGTH", 1000.00f}, //all the program is in MPa ->passed as KPa to vs
     {"DENSITY", 10.00f}, //all the program is in gr/cm3 ->passed as KPa rho * g 
     {"PORE_COLLAPSE",1000.00f } //all the program is in MPa ->passed as KPa to vs
    };

    _plasticity_multiplier.append_value( 0.0, 1.0 );
    _plasticity_multiplier.append_value( 1.0, 1.0 );


}

bool gpm_visage_link::process_ui( string_view json_string )
{

    optional<UIParametersDVTDepthMultipliers> params = JsonParser::parse_json_string<UIParametersDVTDepthMultipliers>( json_string, _visage_options, _output_array_names );//, _plasticity_multiplier, _strain_function );

    if(params.has_value( ))
    {
        _sediments = params->sediments;
        _strain_function = params->strain_function;
        _data_arrays.id( ) = params->properties["FAILUREMODE"];

        _ui_params = std::make_shared< UIParametersDVTDepthMultipliers>( params.value( ) );
        if(UIParametersDVTDepthMultipliers* d = dynamic_cast<UIParametersDVTDepthMultipliers*>(_ui_params.get( )))
        {
            std::cout << "Using DVT-Depth Multipliers" << *d << std::endl;
        }
    }



    //old version of ui parameters 
    //optional<UIParameters> params = JsonParser::parse_json_string<UIParameters>( json_string, _visage_options, _output_array_names );//, _plasticity_multiplier, _strain_function );
    //if(params.has_value( ))
    //{
    //    _sediments = params->sediments;
    //     std::cout << *params << endl;
    //    _plasticity_multiplier = params->plasticity_multiplier;
    //    _strain_function = params->strain_function;
    //}
    return params.has_value( );

}

//the names of the attributes that we need FROM gpm
vector<pair<string, bool>> gpm_visage_link::list_needed_attribute_names( const vector<string>& from_all_atts ) const
{
    vector<pair<string, bool>> to_named_pairs = { {"TOP", false} };

    auto if_condition_met = []( const string name ) { return finder( name, { "SED", "POR","PORO", "SEALEVEL" } ); };
    auto create_pair = []( const string name ) -> pair<string, bool> { return make_pair( name, false ); };

    push_back_selected( from_all_atts, to_named_pairs, if_condition_met, create_pair );

    return to_named_pairs;
}

//the names of the attributes that we will COPY TO GPM 
std::vector<gpm_visage_link::property_type> gpm_visage_link::list_wanted_attribute_names( bool include_top ) const
{
    std::vector<property_type> atts = { { WellKnownVisageNames::ResultsArrayNames::Stiffness, false },  //false means actually 
                                        { WellKnownVisageNames::ResultsArrayNames::Porosity , false } };//not only top surface but all !


    atts.push_back( { "TOTDISX", false } );
    atts.push_back( { "TOTDISY", false } );
    atts.push_back( { "TOTDISZ", false } );


    if(include_top) atts.push_back( { "TOP", false } );

    if(_data_arrays.id( ) != WellKnownVisageNames::FailureMode::ELASTIC)//  if(!_visage_options.enforce_elastic( ))
    {
        atts.push_back( { "EQPLSTRAIN", false } );
        atts.push_back( { "PLSHEARSTRAIN", false } );
    }


    if(MechPropertiesDVT* d = dynamic_cast<MechPropertiesDVT*>(_mech_props_model.get( )))
    {
        cout << "Using dvt tables" << endl;
        atts.push_back( { "CMP" + WellKnownVisageNames::ResultsArrayNames::Stiffness, false } );
    }

    if(MechPropertiesEffectiveMedium* eff = dynamic_cast<MechPropertiesEffectiveMedium*>(_mech_props_model.get( )))
    {
        cout << "Using MechPropertiesEffectiveMedium tables" << endl;
    }

    if(MechPropertiesPlasticityAndDepthDependency* eff = dynamic_cast<MechPropertiesPlasticityAndDepthDependency*>(_mech_props_model.get( )))
    {
        cout << "Using MechPropertiesPlasticityAndDepthDependency " << endl;
    }

    transform( _output_array_names.begin( ), _output_array_names.end( ), back_inserter( atts ),
               []( string name )->property_type { return { name, false }; } );

    return atts;
}

bool gpm_visage_link::update_boundary_conditions( const gpm_attribute& top )//StructuredGeometry &geometry, bool flip_surface_order)
{
    //move the smart pointer. The prev base becomes the base of the last time step
    //and the new base will be figured out below ( queue of two )
    prev_base = std::move( base );
    base.reset( new StructuredSurface( _visage_options->geometry( )->get_structured_surface( 0 ) ) );

    vector<float> gpm_base = get_gpm_heights( top, 0 ); // TODO: slow, improve 
    vector<float>& base_heights = base->heights( );
    copy( gpm_base.begin( ), gpm_base.end( ), base_heights.begin( ) );

    //if I have and old and a new, it means that I am already getting a potentially modified
    //basement.  There is already some history. Lets figure out the displacements to be used as BC.   
    DisplacementSurfaceBoundaryCondition* bc = static_cast<DisplacementSurfaceBoundaryCondition*>(_visage_options->get_boundary_condition( 2 ));
    bc->clear_displacement( );
    if(prev_base)
    {
        //lets see the basement displacement
        vector<float> displacement;
        transform( begin( base->heights( ) ), end( base->heights( ) ), prev_base->heights( ).begin( ),
                   back_inserter( displacement ), []( const float& h1, const float& h2 )
                   {
                       return h1 - h2;
                   } );
        bc->set_node_displacement( displacement );
    }

    const StructuredGrid& geometry = _visage_options->geometry( );
    int long_dir = geometry.lateral_extent( )[0] > geometry.lateral_extent( )[1] ? 0 : 1, short_dir = (1 - long_dir);

    //get the lateral strain from a table;
    float end_time = abs( static_cast<float>(gpm_time.end) );
    float _lateral_strain = _strain_function.get_interpolate( end_time );

    //in this version only strain boundary conditions or "fixed" ones are allowed. 
    StrainBoundaryCondition* long_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( long_dir ));
    StrainBoundaryCondition* short_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( short_dir ));
    long_bc->strain( ) = _lateral_strain;
    short_bc->strain( ) = 0.0f;
    cout << "\n\nImposing boundary strain " << long_bc->strain( ) << " for time " << end_time << " along dir " << long_bc->dir( ) << endl;

    return true;
}

bool  gpm_visage_link::read_visage_results( int last_step, string& error )
{
    if(string file_to_parse = _vs_results_reader.get_results_file( _visage_options->model_name( ), _visage_options->path( ), last_step );
        file_to_parse.empty( ))
    {
        error += "\nError parsing results from geomechanics simulation. X file not found";
        return false;
    }
    else
    {
        vector<string> names_in_file = _vs_results_reader.get_key_names( file_to_parse );
        int total_read = 0; //debug 

        for(auto name_in_file : names_in_file)
        {
            total_read += (1 - _vs_results_reader.read_result( file_to_parse, name_in_file, _data_arrays, &_from_visage_unit_conversion ));
        }
    }

    //here
    if(_data_arrays.contains( "NRCKDISX" ))
    {
        std::string source_names[] = { "NRCKDISX","NRCKDISY","NRCKDISZ" };
        std::string target_names[] = { "TOTDISX","TOTDISY","TOTDISZ" };

        for(int n : {0, 1, 2})
        {
            const vector<float>& disp = _data_arrays.get_array( source_names[n] );
            vector<float>& cum_disp = _data_arrays.get_or_create_array( target_names[n] );
            cum_disp.resize( disp.size( ), 0.0f );
            for(int i = 0; i < cum_disp.size( ); i++) cum_disp[i] += disp[i];
        }
    }

    //some results are derived from others. Example: eq. plastic strain. We will compute it here as well.
    if(_data_arrays.id( ) != WellKnownVisageNames::FailureMode::ELASTIC) //if(!_visage_options.enforce_elastic( ))
    {
        const vector<float>& exx = _data_arrays.get_array( "PLSTRNXX" );
        const vector<float>& eyy = _data_arrays.get_array( "PLSTRNYY" );
        const vector<float>& ezz = _data_arrays.get_array( "PLSTRNZZ" );

        const vector<float>& exy = _data_arrays.get_array( "PLSTRNXY" );
        const vector<float>& eyz = _data_arrays.get_array( "PLSTRNYZ" );
        const vector<float>& ezx = _data_arrays.get_array( "PLSTRNZX" );

        vector<float>& eq = _data_arrays.get_or_create_array( "EQPLSTRAIN" );//, 0.0f, exx.size() );
        vector<float>& plshear = _data_arrays.get_or_create_array( "PLSHEARSTRAIN" );//, 0.0f, exx.size() );
        eq.resize( exx.size( ), 0.0f );
        plshear.resize( exx.size( ), 0.0f );

        float a = 2.0 / 3.0, b = 3.0 / 2.0, one_third = 1.0 / 3.0, three_quaters = 0.75f;
        for(int n : IntRange( 0, exx.size( ) ))
        {
            float vxx = one_third * (2.0f * exx[n] - eyy[n] - ezz[n]);
            float vyy = one_third * (-1.0f * exx[n] + 2.0 * eyy[n] - ezz[n]);
            float vzz = one_third * (-1.0f * exx[n] - eyy[n] + 2.0f * ezz[n]);

            float vxy = (exy[n] * exy[n] + eyz[n] * eyz[n] + ezx[n] * ezx[n]);

            plshear[n] = sqrtf( vxy );
            eq[n] = a * sqrtf( b * (vxx * vxx + vyy * vyy + vzz * vzz) + three_quaters * vxy );
        }
    }
    return true;
}

int  gpm_visage_link::run_visage( string mii_file )
{
    //eclrun visage - v 2018.3 PALEOV3_1.MII

    std::string n_cores = _visage_options->geometry( )->total_nodes( ) < 50000 ? "  --np=1 "
        : _ui_params->contains_prop( "n_cores" ) ? "  --np=" + std::to_string( (int)(_ui_params->get_property( "n_cores" )) ) : "";
    std::cout << "Calling eclrun visage " << mii_file << n_cores << std::endl;
    std::string command = "eclrun visage  " + mii_file + n_cores;// << std::endl;

    int ret_code = system( command.c_str( ) );
    std::cout << "return code  " << ret_code << std::endl;

    return ret_code;
}

int gpm_visage_link::run_timestep( const attr_lookup_type& gpm_attributes, std::string& log, const gpm_plugin_api_timespan& time_span )
{
    std::cout << (!_error ? "[run_timestep] run_timestep counter " + to_string( _time_step ) : "\n\n----skipping simulation of step " + to_string( _time_step ) + "\n\n") << endl;
    if(_error) return 1;//at present, we dont have a way of stopping the GPM engine when we have an error in VS.

    //if(_visage_options.step( ) <= 0)
    //{
    //    std::cout << "Creating debug file " << std::endl;
    //    std::string debug_file = (std::filesystem::path( _visage_options->path( ) ) /= filesystem::path( "DebugFile" )).string( ) + std::to_string( _visage_options.step( ) );
    //    ofstream file( debug_file, ios::out );
    //    file.close( );
    //}

    StructuredGrid& geometry = _visage_options->geometry( );
    const gpm_attribute& top = gpm_attributes.at( "TOP" );
    gpm_time = time_span;


    int old_num_surfaces = geometry->nsurfaces( ), new_num_surfaces = (int)top.size( );
    auto stop_vs_file_path = [this]( ) ->string {return (filesystem::path( _visage_options.path( ) ) /= filesystem::path( "stop_visage" )).string( ); };

    if(geometry->nsurfaces( ) == 0)//we havent even initialized the vs geometry.It must be the 1st step
    {
        //copy exactly the gpm geometry at the first step 
        geometry->set_num_surfaces( new_num_surfaces );
        for(auto k : IntRange( 0, new_num_surfaces ))
        {
            auto [it1, it2] = const_att_iterator::surface_range( top, k );
            std::copy( it1, it2, geometry->begin_surface( k ) );
        }

        _mech_props_model->update_initial_mech_props( gpm_attributes, _sediments, _visage_options, _data_arrays, 0, new_num_surfaces );
        old_num_surfaces = new_num_surfaces;

        //remove the stop file at the 1st step if present 
        int code = std::remove( stop_vs_file_path( ).c_str( ) );
    }

    if(std::filesystem::exists( stop_vs_file_path( ) ))
    {
        _error = true;
        return 1;
    }

    update_boundary_conditions( top );
    cout << "Done boundary conditions" << endl;

    //add the new surface(s) preserving gpm thickness deposited. 
    vector<float> nodal_thickness;
    for(int k : IntRange( old_num_surfaces, new_num_surfaces ))
    {
        geometry->set_num_surfaces( 1 + geometry->nsurfaces( ) ); //old surfaces not modified, new not initialized.

        nodal_thickness.resize( top[0].num_cols( ) * top[0].num_rows( ) );
        auto [beg_above, end_above, beg_below] = const_att_iterator::surface_range( top, k, k - 1 );
        std::transform( beg_above, end_above, beg_below, begin( nodal_thickness ), []( float h1, float h2 ) { return fabs( h1 - h2 ); } );//  std::minus<float>( ) );
        const auto& zbelow = geometry->get_local_depths( k - 1 );
        auto& zabove = geometry->get_local_depths( k );

        for(int n : IntRange( 0, zabove.size( ) ))
        {
            zabove[n] = zbelow[n] + nodal_thickness[n];
        }

    }
    cout << "Done nodal thickness" << endl;

    _mech_props_model->update_initial_mech_props( gpm_attributes, _sediments, _visage_options, _data_arrays, old_num_surfaces, new_num_surfaces );

    cout << "Done mech props" << endl;

    int nprops = _data_arrays.count( );

    increment_step( );

    /*
    skip some cycles
    */
    bool dummy_run = false;
    if(_ui_params->contains_prop( "skip_steps" ))
    {
        int skip = (int)(_ui_params->get_property( "skip_steps" ));
        dummy_run = !(_visage_options->step( ) % skip == 0);
        cout << "Is this a dummy run?  " << boolalpha <<  dummy_run << endl;
    }

    /*
    CREATE NOTCH, ALWAYS     
    */
    {
     int ele_per_layers = std::max<int>(_visage_options->geometry()->ncols(),_visage_options->geometry( )->nrows( ) )- 1;
     int delta  = (int)(ele_per_layers / 6.0);
     int n1 = 2 * delta ;
     int n2 = n1 + delta ;
     auto& v = _data_arrays[ WellKnownVisageNames::ResultsArrayNames::Stiffness ];
     for(int n = n1; n <= n2; n++) v[n] = 0.1f;
     
     v = _data_arrays["POISSONR"];
     for(int n = n1; n <= n2; n++) v[n] = 0.1f;

     v = _data_arrays["COHESION"];
     for(int n = n1; n <= n2; n++) v[n] = 99999999.0f;
     
     v = _data_arrays["TENSILE_STRENGTH"];
     for(int n = n1; n <= n2; n++) v[n] = 99999999.0f;


    }

    



















    int run_attempt = 1;
    int max_attempts = 20;

    cout << "About to write mii file" << std::endl;
    string mii_file_name = VisageDeckWritter::write_deck( &_visage_options, &_data_arrays, &_to_visage_unit_conversion, dummy_run );
    cout << "Done writting mii file " << mii_file_name << endl;

    while((run_attempt <= max_attempts) && (run_visage( mii_file_name ) != 0))
    {
        cout << "Visage run failed.  MII file: " << mii_file_name << " recovery attempt " << run_attempt << std::endl;
        run_attempt += 1;

        ////get the mio and make a copy of it. 
        //try {
        //    string mio = VisageDeckWritter::get_mio_filename( _visage_options );
        //    filesystem::copy( mio, mio + to_string( run_attempt ) );
        //}
        //catch(...) {}


    }
    if(run_attempt >= max_attempts)
    {
        log += ("Visage run failed.  MII file: " + mii_file_name);
        cout << "Visage run failed.  MII file: " << mii_file_name << " recovery attempt " << run_attempt << std::endl;
        _error = true;
    }
    else
    {
        cout << "Visage run succeeded. " << mii_file_name << " Attempt: " << run_attempt << std::endl;
    }
    return _error ? 1 : 0;
}

bool gpm_visage_link::update_gpm_and_visage_geometris_from_visage_results( map<string, gpm_attribute>& attributes, string& error )
{
    //basically, we modify now the property "TOP" using the cummulated displacements in visage.
    if(attributes.find( "TOP" ) == attributes.end( ))
    {
        error += "[update_gpm_geometry_from_visage] Attribute TOP was not found ";
        cout << error << endl;
        _error = true;
        return false;
        int step = _visage_options.step( );



    }

    StructuredGrid& geometry = _visage_options->geometry( );
    auto [ncols, nrows, nsurfaces, total_nodes, total_elements] = geometry->get_geometry_description( );

    //nodal_values.resize( total_nodes );

    //if we have nodal displacements, use them
    if(_data_arrays.contains( "NRCKDISZ" ))
    {
        geometry->displace_all_nodes( _data_arrays.get_array( "NRCKDISZ" ) );
    }

    //if we only have elemenal, take them and convert to nodal 
    else if(_data_arrays.contains( "ROCKDISZ" ))
    {
        std::vector<float> nodal_values = StructuredBase::elemental_to_nodal( ncols, nrows, nsurfaces, _data_arrays.get_array( "ROCKDISZ" ) );
        geometry->displace_all_nodes( nodal_values );
    }

    //if we have neither, exit with an error code. 
    else
    {
        error += "\n[update_gpm_geometry_from_visage] The number of displacement values does not match the number of nodes when reading X files";
        cout << error << endl;
        _error = true;
        return false;
    }

    //now the visage part NOT CUMMULATIVE.
    //geometry->displace_all_nodes( nodal_values );

    //this is the gpm part. debug: at this point, base in vs must be identical to base in gpm
    gpm_attribute& top = attributes.at( "TOP" );
    for(int k : IntRange( 0, nsurfaces ))
    {
        auto [vs_it1, vs_it2] = geometry.surface_range( k );

        auto [gpm_it1, _] = att_iterator::surface_range( top, k );

        std::copy( vs_it1, vs_it2, gpm_it1 );
    }


    //std::string debug_file = (std::filesystem::path( _visage_options->path( ) ) /= filesystem::path( "DebugFile" )).string( ) + std::to_string( _visage_options.step( ) );
    //if(ofstream file( debug_file, ios::app ); file)
    //{

    //    const StructuredGrid& geometry = _visage_options->geometry( );
    //    for(int k1 = 0; k1 < geometry.nsurfaces( ) - 1; k1++)
    //    {
    //        int k2 = k1 + 1;
    //        const vector<float>& heights_base = geometry.get_local_depths( k1 );
    //        const vector<float>& heights_above = geometry.get_local_depths( k2 );

    //        int nodes_per_surface = (int)heights_above.size( );

    //        for(auto n : IntRange( 0, nodes_per_surface ))
    //        {
    //            if(heights_above < heights_base)
    //            {
    //                int node1 = nodes_per_surface * k1 + n;
    //                int node2 = node1 + nodes_per_surface;

    //                file << "[update_gpm_and_visage_geometris_from_visage_results] Found traspased node " << node1 << " " << node2 << endl;
    //            }
    //        }
    //    }



    //    file.close( );
    //}















    return true;
}

void   gpm_visage_link::update_compacted_props( attr_lookup_type& attributes )
{
    _mech_props_model->update_compacted_props( attributes, _sediments, _visage_options, _data_arrays, _plasticity_multiplier );
}

int   gpm_visage_link::update_results( attr_lookup_type& attributes, std::string& error, int step )
{
    read_visage_results( _time_step, error );

    update_gpm_and_visage_geometris_from_visage_results( attributes, error );

    update_compacted_props( attributes );

    //now we should copy whatever results we need to copy from vs to gpm for display 
    bool include_top = false;
    auto to_copy = list_wanted_attribute_names( include_top );
    const auto& geometry = _visage_options.geometry( );
    auto [ncols, nrows, nsurfaces, total_nodes, total_elements] = geometry.get_geometry_description( );


    //copy props to gpm for display 
    for(const auto& vs_prop : to_copy)
    {
        vector<float>& values = _data_arrays[vs_prop.name];
        vector<float>* aux = nullptr;
        auto to_gpm = att_iterator( attributes[vs_prop.name] );

        if(_data_arrays.array_size( vs_prop.name ) == geometry.total_nodes( ))
        {
            copy( values.begin( ), values.end( ), to_gpm );
        }

        else if(_data_arrays.array_size( vs_prop.name ) == geometry.total_elements( ))
        {
            vector<float> nodal_values = StructuredBase::elemental_to_nodal( ncols, nrows, nsurfaces, values );

            if((vs_prop.name.find( "STRAIN" ) != string::npos) || (vs_prop.name.find( "STRN" ) != string::npos))
            {
                for_each( begin( nodal_values ), end( nodal_values ), []( float& v ) {v *= 1.0e5; } );
            }
            if(vs_prop.name.find( "EFFSTR" ) != string::npos)
            {
                for_each( begin( nodal_values ), end( nodal_values ), []( float& v ) {v *= 1.0e5; } );
            }

            copy( nodal_values.begin( ), nodal_values.end( ), to_gpm );
        }
        else { continue; } //empty values, not computed, etc...etc...       
    }


    return 0;
}