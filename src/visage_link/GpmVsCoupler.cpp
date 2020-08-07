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
///
#include "Vector3.h"
#include "Range.h"
#include "StructuredGrid.h"
#include "utils.h"
#include "GpmVsCoupler.h"
#include "gpm_visage_results.h"
#include "gpm_plugin_description.h"
#include "gpm_plugin_helpers.h"

using namespace std;


int gpm_visage_link::run_timestep( const attr_lookup_type& gpm_attributes, std::string& log )
{
    std::cout << (!_error ? "[run_timestep] run_timestep counter " + to_string( _time_step ) : "\n\n----skipping simulation of step " + to_string( _time_step ) + "--------\n\n") << endl;

    //at present, we dont have a way of stopping the GPM engine when we have an error in VS.
    if(_error) return 1;

    StructuredGrid& geometry = _visage_options->geometry( );
    const gpm_attribute& top = gpm_attributes.at( "TOP" );

    int old_num_surfaces = geometry->nsurfaces( ), new_num_surfaces = (int)top.size( );

    if(geometry->nsurfaces( ) == 0)//we havent even initialized the vs geometry.It must be the 1st step
    {
        //copy exactly the gpm geometry at the first step 
        geometry->set_num_surfaces( new_num_surfaces );
        for(auto k : IntRange( 0, new_num_surfaces ))
        {
            auto[it1, it2] = const_att_iterator::surface_range( top, k );
            std::copy( it1, it2, geometry->begin_surface( k ) );
        }
        update_mech_props( gpm_attributes, old_num_surfaces, new_num_surfaces );
        old_num_surfaces = new_num_surfaces;
    }

    //add the new surface(s) preserving gpm thickness deposited. 
    vector<float> nodal_thickness;
    for(int k : IntRange( old_num_surfaces, new_num_surfaces ))
    {
        geometry->set_num_surfaces( 1 + geometry->nsurfaces( ) ); //old surfaces not modified, new not initialized.
        nodal_thickness.resize( top[0].num_cols( ) * top[0].num_rows( ) );

        auto[beg_above, end_above, beg_below] = const_att_iterator::surface_range( top, k, k - 1 );
        std::transform( beg_above, end_above, beg_below, begin( nodal_thickness ), std::minus<float>( ) );
        const auto& zbelow = geometry->get_local_depths( k - 1 );
        std::transform( cbegin( zbelow ), cend( zbelow ), cbegin( nodal_thickness ), geometry->begin_surface( k ), []( float h1, float dh ) {return h1 + dh; } );
    }


    update_boundary_conditions( );
    update_mech_props( gpm_attributes, old_num_surfaces, new_num_surfaces );
    increment_step( );
    string mii_file_name = VisageDeckWritter::write_deck( &_visage_options, &_data_arrays );
    //if(run_visage( mii_file_name ) != 0)
    //{
    //    log += ("Visage run failed.  MII file: " + mii_file_name);
    //    _error = true;
    //}


    return _error ? 1 : 0;
}



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
    std::vector<property_type> atts = { { "CMP_YMOD", false }, { "CMP_POROSITY", false } };
    if(include_top) atts.push_back( { "TOP", false } );

    transform( _output_array_names.begin( ), _output_array_names.end( ), back_inserter( atts ),
               []( string name )->property_type { return { name, false }; } );

    return atts;
}

void gpm_visage_link::initialize_model_extents( const gpm_plugin_api_model_definition* model_def )
{
    cout << "Initializing vs geometry extents" << endl;

    const float* x = model_def->x_coordinates;
    const float* y = model_def->y_coordinates;
    fVector3 axis1( x[1] - x[0], y[1] - y[0], 0.0f );
    fVector3 axis2( x[3] - x[0], y[3] - y[0], 0.0f );
    fVector3 axis3( 0.0f, 0.0f, 1.0f );
    fVector2 extent( axis1.length( ), axis2.length( ) );

    int ncols = (int)model_def->num_columns, nrows = (int)model_def->num_rows;
    CoordinateMapping3D reference( axis1.normalize( ), axis2.normalize( ), axis3, { 0.0f,0.0f,0.0f } );

    _visage_options->geometry( ) = StructuredGrid( ncols, nrows, 0, extent, reference );
    cout << _visage_options->geometry( ) << std::endl;
}

bool gpm_visage_link::process_ui( string json_string )
{
    optional<UIParameters> params = JsonParser::parse_json_string<UIParameters>( json_string, _visage_options, _output_array_names );

    if(params.has_value( ))
    {
        sediments = params->sediments;
        cout << *params << endl;
    }

    _lateral_strain = params->properties.at( "LateralStrain" );

    return params.has_value( );
}

bool gpm_visage_link::update_boundary_conditions( )//const gpm_attribute& top )//StructuredGeometry &geometry, bool flip_surface_order)
{
    const StructuredGrid& geometry = _visage_options->geometry( );
    int long_dir = geometry.lateral_extent( )[0] > geometry.lateral_extent( )[1] ? 0 : 1;
    int short_dir = (1 - long_dir);

    StrainBoundaryCondition* long_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( long_dir ));
    StrainBoundaryCondition* short_bc = static_cast<StrainBoundaryCondition*>(_visage_options->get_boundary_condition( short_dir ));
    long_bc->strain( ) = _lateral_strain;
    short_bc->strain( ) = 0.0f;

    DisplacementSurfaceBoundaryCondition* base_bc = static_cast<DisplacementSurfaceBoundaryCondition*>(_visage_options->get_boundary_condition( 2 ));
    base_bc->clear_displacement( );

    prev_base = std::move( base );
    base.reset( new StructuredSurface( _visage_options->geometry( )->get_structured_surface( 0 ) ) );

    //if I have and old and a new-> there is already some history. Lets figure out the displacements to be used as BC.
    if(prev_base)
    {
        base_bc->set_node_displacement( *base - *prev_base );
    }

    return true;
}

//bool  gpm_visage_link::read_visage_results( int last_step, string& error )
//{
//    string file_to_parse = _vs_results_reader.get_results_file( _visage_options->model_name( ), _visage_options->path( ), last_step );
//    if(!file_to_parse.empty( ))
//    {
//        error += "\nError parsing results from geomechanics simulation. X file not found";
//        return false;
//    }
//
//    std::vector<gpm_visage_link::property_type> props_wanted = list_wanted_attribute_names( false );
//    //vector<string> names = _vs_results_reader.get_key_names( file_to_parse );
//
//    int total_read = 0;
//    if(!file_to_parse.empty( ))
//    {
//        map<string, float> unit_conversion = { {WellKnownVisageNames::ResultsArrayNames::Stiffness,1.00f} }; //FIXME
//        for(auto prop : props_wanted)
//        {
//            string name_in_file = prop.name;
//            string name_here = prop.name == WellKnownVisageNames::ResultsArrayNames::Stiffness ? "CMP" + WellKnownVisageNames::ResultsArrayNames::Stiffness : prop.name;
//            total_read += (1 - _vs_results_reader.read_result( file_to_parse, name_in_file, _data_arrays, &unit_conversion, name_here ));
//        }
//    }
//
//    //some results are derived from others. Example: porosity. We will compute it here as well.
//    vector<float>& cmp_porosity = _data_arrays.get_array( "CMP_POROSITY" );
//    vector<float>& initial_porosity = _data_arrays.get_array( "POROSITY" );
//
//    //the total volumetric strain: assume it is cummulative.
//    vector<float>& exx = _data_arrays.get_array( "STRAINXX" );
//    vector<float>& eyy = _data_arrays.get_array( "STRAINYY" );
//    vector<float>& ezz = _data_arrays.get_array( "STRAINZZ" );
//
//    //for (auto n : IntRange(0, ezz.size()))
//    //{
//    //    float delta = (-exx[n] - eyy[n] - ezz[n]);
//    //    cmp_porosity[n] = std::min<float>(0.8f, std::max<float>(0.01f, initial_porosity[n] + delta));
//    //}
//
//    return true;
//}

int  gpm_visage_link::run_visage( string mii_file )
{
    std::cout << "Calling eclrun visage " << mii_file << "  --np=4" << std::endl;
    std::string command = "eclrun visage  " + mii_file + "   --np=4";
    int ret_code = system( command.c_str( ) );
    std::cout << "return code  " << ret_code << std::endl;

    return ret_code;
}

void gpm_visage_link::update_initial_mech_props( const attr_lookup_type& atts, int old_nsurf, int new_nsurf )
{
    if(old_nsurf == new_nsurf) return;

    vector<string> sed_keys = {}; //SED1,SED2,...SEDN  
    for_each( cbegin( atts ), cend( atts ), [&sed_keys, key = "SED"]( const auto& att )
    {if(att.first.find( key ) != std::string::npos) sed_keys.push_back( att.first ); } );

    int tot_nodes = (atts.at( "TOP" ).size( ) * atts.at( "TOP" )[0].num_cols( ) * atts.at( "TOP" )[0].num_rows( ));
    vector<float> value( tot_nodes );

    set<string> prop_names = { sediments.at( sed_keys[0] ).property_names( ) }; //"POROSITY", "YOUNGMOD",......etc...")
    for(string prop : prop_names)//
    {
        fill( value.begin( ), value.end( ), 0.0f );
        for(string key : sed_keys)
        {
            vector<float> weights = get_values( atts.at( key ), 0, new_nsurf );
            transform( begin( weights ), end( weights ), begin( weights ), [val = sediments.at( key ).properties.at( prop )]( float& v ){ return v * val; } );
            for(auto n : IntRange( 0, tot_nodes )) value[n] += (weights[n]);
        }

        //value is the volume-weighted average of property = prop (nodal in gpm) 
        auto[vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = _visage_options->geometry( )->get_geometry_description( );
        int offset = (vs_cols - 1) * (vs_rows - 1) * (old_nsurf > 0 ? (old_nsurf - 1) : 0);

        auto& data_array = _data_arrays[prop];//.get_or_create_array( prop );
        vector<float> ele_values = geometry( )->nodal_to_elemental( value );
        if(data_array.size( ) != vs_total_elements)
        {
            data_array.resize( vs_total_elements, 0.0f );
        }

        copy( ele_values.begin( ) + offset, ele_values.end( ), data_array.begin( ) + offset );
    }


}

void gpm_visage_link::update_compacted_props( const attr_lookup_type& atts, int old_nsurf, int new_nsurf )
{
    if(old_nsurf == new_nsurf) return;

    vector<string> sed_keys = {}; //SED1,SED2,...SEDN  
    for_each( cbegin( atts ), cend( atts ), [&sed_keys, key = "SED"]( const auto& att )
    {if(att.first.find( key ) != std::string::npos) sed_keys.push_back( att.first ); } );

    //compaction tables for each element -> the one for sediment of highest concentration there. 

    _visage_options->use_tables( ) = true;
    for(int n : IntRange( 0, sed_keys.size( ) )) _visage_options.add_table( n, sediments[sed_keys[n]].compaction_table );

    vector<pair<float, int>> temp;
    for(int n : IntRange( 0, sed_keys.size( ) ))
    {
        //vector<float> weight = get_values( atts.at( sed_keys[n] ), 0, new_nsurf );
        vector<float> ele_weighted_index = geometry( )->nodal_to_elemental( get_values( atts.at( sed_keys[n] ), 0, new_nsurf ) );
        if(n == 0)
        {
            transform( ele_weighted_index.begin( ), ele_weighted_index.end( ), back_inserter( temp ),
                       []( float w )->pair<float, int> { return { w, 0 }; } );
        }
        else
        {
            for(int i : IntRange( 0, ele_weighted_index.size( ) ))
                if(ele_weighted_index[i] > temp[i].first) temp[i] = { ele_weighted_index[i], n };
        }
    }
    auto& table_index = _data_arrays["dvt_table_index"];//.get_or_create_array( prop );
    table_index.resize( temp.size( ), 0 );
    transform( temp.begin( ), temp.end( ), table_index.begin( ), []( pair<int, float> p ) { return p.first; } );
}

//will copy all the properties stores i the SEDS properties, weighted by sed concentration. 
void gpm_visage_link::update_mech_props( const attr_lookup_type& atts, int old_nsurf, int new_nsurf )
{
    update_initial_mech_props( atts, old_nsurf, new_nsurf );
    update_compacted_props( atts, old_nsurf, new_nsurf );
}
