#include <iostream>
#include <fstream>
#include <string>
#include <iterator>
#include <algorithm>
#include <functional>
#include <tuple>
#include <vector>
#include <map>
#include <thread>
#include <future>
#include <cmath>
#include <filesystem>

#include "Vector3.h"
#include "Range.h"
#include "StructuredGrid.h"
#include "utils.h"
#include "GpmVsCoupler.h"
#include "gpm_visage_results.h"
#include "gpm_plugin_description.h"
#include "gpm_plugin_helpers.h"

using namespace std;


vector<pair<string, bool>> gpm_visage_link::list_needed_attribute_names( const vector<string>& from_all_atts ) const
{
    vector<pair<string, bool>> to_named_pairs = { {"TOP", false} };

    auto if_condition_met = []( const string name ) { return finder( name, { "SED", "POR","PORO", "SEALEVEL" } ); };
    auto after_transformation = []( const string name ) -> pair<string, bool> { return make_pair( name, false ); };

    push_back_selected( from_all_atts, to_named_pairs, if_condition_met, after_transformation );

    return to_named_pairs;
}


std::vector<gpm_visage_link::property_type> gpm_visage_link::list_wanted_attribute_names( bool include_top ) const
{
    std::vector<property_type> atts = { { WellKnownVisageNames::ResultsArrayNames::Stiffness, false },
                                        { WellKnownVisageNames::ResultsArrayNames::Porosity , false } };

    if(include_top) atts.push_back( { "TOP", false } );
    if(!_visage_options.enforce_elastic( ))
    {
        atts.push_back( { "EQPLSTRAIN", false } );
    }
    if( static_cast<MechPropertiesDVT*>(_mech_props_model.get()))
    {
     cout<<"Using dvt tables"<<endl;
     atts.push_back( { "CMP" + WellKnownVisageNames::ResultsArrayNames::Stiffness, false } );
    }
    if(static_cast<MechPropertiesEffectiveMedium*>(_mech_props_model.get( )))
    {
        cout << "Using MechPropertiesEffectiveMedium tables" << endl;
    }



    transform( _output_array_names.begin( ), _output_array_names.end( ), back_inserter( atts ),
               []( string name )->property_type { return { name, false }; } );

    return atts;
}

bool gpm_visage_link::process_ui( string json_string )
{
    optional<UIParameters> params = JsonParser::parse_json_string<UIParameters>( json_string, _visage_options, _output_array_names );//, _plasticity_multiplier, _strain_function );

    if(params.has_value( ))
    {
        _sediments = params->sediments;
        cout << *params << endl;
    }

    _plasticity_multiplier = params->plasticity_multiplier;
    _strain_function = params->strain_function;

    cout << "\n\nplasticity multiplier " << _plasticity_multiplier << endl;

    cout << "\n\nstrain function " << _strain_function << endl;

    return params.has_value( );
}

bool gpm_visage_link::update_boundary_conditions( const gpm_attribute& top )//StructuredGeometry &geometry, bool flip_surface_order)
{
    prev_base = std::move( base );
    base.reset( new StructuredSurface( _visage_options->geometry( )->get_structured_surface( 0 ) ) );

    vector<float> gpm_base = get_gpm_heights( top, 0 );
    vector<float>& base_heights = base->heights( );
    copy( gpm_base.begin( ), gpm_base.end( ), base_heights.begin( ) );

    //if I have and old and a new, it means that I am already getting a potentially modified
    //basement.  There is already some history. Lets figure out the displacements to be used as BC.
    vector<float> displacement;
    DisplacementSurfaceBoundaryCondition* bc = static_cast<DisplacementSurfaceBoundaryCondition*>(_visage_options->get_boundary_condition( 2 ));
    bc->clear_displacement( );
    if(prev_base)
    {
        //lets see the basement displacement
        transform( begin( base->heights( ) ), end( base->heights( ) ), prev_base->heights( ).begin( ),
                   back_inserter( displacement ), []( const float& h1, const float& h2 )
                   {
                       return h1 - h2;
                   } );
        bc->set_node_displacement( displacement );
    }
    else
    {
        bc->clear_displacement( );
        displacement.resize( base->total_nodes( ), 0.0f );
    }


    const StructuredGrid& geometry = _visage_options->geometry( );
    int long_dir = geometry.lateral_extent( )[0] > geometry.lateral_extent( )[1] ? 0 : 1;
    int short_dir = (1 - long_dir);

    //get the lateral strain from a table;
    float end_time = abs( static_cast<float>(gpm_time.end) );
    float _lateral_strain = _strain_function.get_interpolate( end_time );

    StrainBoundaryCondition* long_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( long_dir ));
    StrainBoundaryCondition* short_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( short_dir ));
    long_bc->strain( ) = _lateral_strain;
    short_bc->strain( ) = 0.0f;
    cout << "\n\nimposing boundary strain " << long_bc->strain( ) << " time " << end_time << " dir " << long_bc->dir( ) << endl;

    return true;
}

bool  gpm_visage_link::read_visage_results( int last_step, string& error )
{
    string file_to_parse = _vs_results_reader.get_results_file( _visage_options->model_name( ), _visage_options->path( ), last_step );
    if(file_to_parse.empty( ))
    {
        error += "\nError parsing results from geomechanics simulation. X file not found";
        return false;
    }

    vector<string> names_in_file = _vs_results_reader.get_key_names( file_to_parse );
    int total_read = 0;
    if(!file_to_parse.empty( ))
    {

        for(auto name_in_file : names_in_file)
        {
            total_read += (1 - _vs_results_reader.read_result( file_to_parse, name_in_file, _data_arrays, &_from_visage_unit_conversion ));
        }
    }

    //some results are derived from others. Example: eq. plastic strain. We will compute it here as well.
    if(!_visage_options.enforce_elastic( ))
    {
        vector<float>& exx = _data_arrays.get_array( "PLSTRNXX" );
        vector<float>& eyy = _data_arrays.get_array( "PLSTRNYY" );
        vector<float>& ezz = _data_arrays.get_array( "PLSTRNZZ" );

        vector<float>& exy = _data_arrays.get_array( "PLSTRNXY" );
        vector<float>& eyz = _data_arrays.get_array( "PLSTRNYZ" );
        vector<float>& ezx = _data_arrays.get_array( "PLSTRNZX" );

        vector<float>& eq = _data_arrays.get_or_create_array( "EQPLSTRAIN" );//, 0.0f, exx.size() );
        eq.resize( exx.size( ), 0.0f );

        float a = 2.0 / 3.0, b = 3.0 / 2.0, one_third = 1.0 / 3.0, three_quaters = 0.75f;
        for(int n : IntRange( 0, exx.size( ) ))
        {
            float vxx = one_third * (2.0f * exx[n] - eyy[n] - ezz[n]);
            float vyy = one_third * (-1.0f * exx[n] + 2.0 * eyy[n] - ezz[n]);
            float vzz = one_third * (-1.0f * exx[n] - eyy[n] + 2.0f * ezz[n]);

            float vxy = (exy[n] * exy[n] + eyz[n] * eyz[n] + ezx[n] * ezx[n]);


            eq[n] = a * sqrtf( b * (vxx * vxx + vyy * vyy + vzz * vzz) + three_quaters * vxy );
        }
    }
    return true;
}

int  gpm_visage_link::run_visage( string mii_file )
{
    std::cout << "Calling eclrun visage " << mii_file << " --np=4" << std::endl;
    std::string command = "eclrun visage  " + mii_file + " --np=4";
    int ret_code = system( command.c_str( ) );
    std::cout << "return code  " << ret_code << std::endl;

    return ret_code;
}

int gpm_visage_link::run_timestep( const attr_lookup_type& gpm_attributes, std::string& log, const gpm_plugin_api_timespan& time_span )
{
    std::cout << (!_error ? "[run_timestep] run_timestep counter " + to_string( _time_step ) : "\n\n----skipping simulation of step " + to_string( _time_step ) + "--------\n\n") << endl;
    //at present, we dont have a way of stopping the GPM engine when we have an error in VS.
    if(_error) return 1;

    shared_ptr<ChronoPoint> timer = make_shared<ChronoPoint>( "run_time_step" );
   
    StructuredGrid& geometry = _visage_options->geometry( );
    const gpm_attribute& top = gpm_attributes.at( "TOP" );
    gpm_time = time_span;

    int old_num_surfaces = geometry->nsurfaces( ), new_num_surfaces = (int)top.size( );

    auto stop_vs_file_path = [this]( ) ->string {return (filesystem::path( _visage_options.path( ) ) /= filesystem::path( "stop_visage" )).string( ); };  // lambda expression

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

        try {
            cout << boolalpha << "stop_visage file was removed? " << (std::remove( stop_vs_file_path( ).c_str( ) ) == 0) << endl;
        }
        catch(...) {
            ;
        }
    }

    if(std::filesystem::exists( stop_vs_file_path( ) ))
    {
        _error = true;
        return 1;
    }



    update_boundary_conditions( top );

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

        float zmax = -9999999.99f;
        for(int n : IntRange( 0, zabove.size( ) ))
        {
            zabove[n] = zbelow[n] + nodal_thickness[n];
            if(zabove[n] > zmax) zmax = zabove[n];
        }

    }


    _mech_props_model->update_initial_mech_props( gpm_attributes, _sediments, _visage_options, _data_arrays, old_num_surfaces, new_num_surfaces );

    int nprops = _data_arrays.count( );

    timer = make_shared<ChronoPoint>( "Deck writting" );

    //update_mech_props( gpm_attributes, old_num_surfaces, new_num_surfaces );
    increment_step( );
    string mii_file_name = VisageDeckWritter::write_deck( &_visage_options, &_data_arrays, &_to_visage_unit_conversion );

    timer = make_shared<ChronoPoint>( "Visage running" );
    if(run_visage( mii_file_name ) != 0)
    {
        log += ("Visage run failed.  MII file: " + mii_file_name);
        _error = true;
    }
    timer.reset( );

    return _error ? 1 : 0;
}

bool gpm_visage_link::update_gpm_and_visage_geometris_from_visage_results( map<string, gpm_attribute>& attributes, string& error )
{
    //basically, we modify now the property "TOP" using the cummulated displacements in visage.
    if(attributes.find( "TOP" ) == attributes.end( ))
    {
        error += "[update_gpm_geometry_from_visage] Attribute TOP was not found "; cout << error << endl;
        _error = true;
        return false;
    }

    std::vector<float> nodal_values;
    StructuredGrid& geometry = _visage_options->geometry( );
    auto [ncols, nrows, nsurfaces, total_nodes, total_elements] = geometry->get_geometry_description( );

    nodal_values.resize( total_nodes );
    if(_data_arrays.contains( "NRCKDISZ" ))
    {
        std::vector<float>& values = (_data_arrays.get_array( "NRCKDISZ" ));
        copy( begin( values ), end( values ), begin( nodal_values ) );
    }
    else if(_data_arrays.contains( "ROCKDISZ" ))
    {
        std::vector<float>& values = (_data_arrays.get_array( "ROCKDISZ" ));
        nodal_values = StructuredBase::elemental_to_nodal( ncols, nrows, nsurfaces, values );//, values);
    }
    else
    {
        error += "\n[update_gpm_geometry_from_visage] The number of values read does not match elements or nodes when reading X files";
        cout << error << endl;
        _error = true;
        return false;
    }

    //now the visage part NOT CUMMULATIVE.
    geometry->displace_all_nodes( nodal_values );

    //this is the gpm part. debug: at this point, base in vs must be identical to base in gpm
    gpm_attribute& top = attributes.at( "TOP" );
    for(int k : IntRange( 1, nsurfaces ))
    {
        auto [vs_it1, vs_it2] = geometry.surface_range( k );

        auto [gpm_it1, _] = att_iterator::surface_range( top, k );

        std::copy( vs_it1, vs_it2, gpm_it1 );
    }

    return true;
}

void   gpm_visage_link::update_compacted_props( attr_lookup_type& attributes )
{
    _mech_props_model->update_compacted_props( attributes, _sediments, _visage_options, _data_arrays, _plasticity_multiplier );
}

 



int   gpm_visage_link::update_results( attr_lookup_type& attributes, std::string& error, int step )
{
    read_visage_results( _time_step, error );

    update_compacted_props( attributes );

    update_gpm_and_visage_geometris_from_visage_results( attributes, error );

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

            if( (vs_prop.name.find("STRAIN") != string::npos) || (vs_prop.name.find( "STRN" ) != string::npos))
            {
             //cout<<"Scaling strains by 1E5"<<endl;
             //times 1e5
             for_each( begin(nodal_values), end(nodal_values), []( float &v){v*=1.0e5;} );
            }
            if(vs_prop.name.find( "EFFSTR" ) != string::npos) 
            {
                //cout << "Scaling effective stress by 1E5" << endl;
                //times 1e5
                for_each( begin( nodal_values ), end( nodal_values ), []( float& v ) {v *= 1.0e5; } );
            }

            copy( nodal_values.begin( ), nodal_values.end( ), to_gpm );
        }
        else { continue; } //empty values, not computed, etc...etc...       
    }


    return 0;
}