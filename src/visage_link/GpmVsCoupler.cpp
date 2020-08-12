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
    std::vector<property_type> atts;// = { { "CMP_YMOD", false }, { "CMP_POROSITY", false } };
    if(include_top) atts.push_back( { "TOP", false } );

    transform( _output_array_names.begin( ), _output_array_names.end( ), back_inserter( atts ),
               []( string name )->property_type { return { name, false }; } );

    return atts;
}

bool gpm_visage_link::process_ui( string json_string )
{
    optional<UIParameters> params = JsonParser::parse_json_string<UIParameters>( json_string, _visage_options, _output_array_names );

    if(params.has_value( ))
    {
        _sediments = params->sediments;
        cout << *params << endl;
    }


    _visage_options->enforce_elastic( ) = true;


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

       //some results are derived from others. Example: porosity. We will compute it here as well.
       //vector<float>& cmp_porosity = _data_arrays.get_array( "CMP_POROSITY" );
       //vector<float>& initial_porosity = _data_arrays.get_array( "POROSITY" );

       ////the total volumetric strain: assume it is cummulative.
       //vector<float>& exx = _data_arrays.get_array( "STRAINXX" );
       //vector<float>& eyy = _data_arrays.get_array( "STRAINYY" );
       //vector<float>& ezz = _data_arrays.get_array( "STRAINZZ" );

       //for (auto n : IntRange(0, ezz.size()))
       //{
       //    float delta = (-exx[n] - eyy[n] - ezz[n]);
       //    cmp_porosity[n] = std::min<float>(0.8f, std::max<float>(0.01f, initial_porosity[n] + delta));
       //}

    return true;
}

int  gpm_visage_link::run_visage( string mii_file )
{
    std::cout << "Calling eclrun visage " << mii_file << " -np=4" << std::endl;
    std::string command = "eclrun visage  " + mii_file + " -np=4";
    int ret_code = system( command.c_str( ) );
    std::cout << "return code  " << ret_code << std::endl;

    return ret_code;
}



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
        _mech_props_model->update_initial_mech_props( gpm_attributes, _sediments, _visage_options, _data_arrays, 0, new_num_surfaces );
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
    _mech_props_model->update_initial_mech_props( gpm_attributes, _sediments, _visage_options, _data_arrays, old_num_surfaces, new_num_surfaces );

    int nprops = _data_arrays.count( );


    //update_mech_props( gpm_attributes, old_num_surfaces, new_num_surfaces );
    increment_step( );
    string mii_file_name = VisageDeckWritter::write_deck( &_visage_options, &_data_arrays, &_to_visage_unit_conversion );
    if(run_visage( mii_file_name ) != 0)
    {
        log += ("Visage run failed.  MII file: " + mii_file_name);
        _error = true;
    }


    return _error ? 1 : 0;
}



bool gpm_visage_link::update_gpm_and_visage_geometris_from_visage_results( map<string, gpm_attribute>  &attributes, string &error )
{
    //basically, we modify now the property "TOP" using the cummulated displacements in visage.
    if(attributes.find( "TOP" ) == attributes.end( ))
    {
        error += "[update_gpm_geometry_from_visage] Attribute TOP was not found ";cout << error << endl;
        _error = true;
        return false;
    }

    std::vector<float> nodal_values;
    StructuredGrid &geometry = _visage_options->geometry( );
    auto[ncols, nrows, nsurfaces, total_nodes, total_elements] = geometry->get_geometry_description( );

    nodal_values.resize( total_nodes );
    if(_data_arrays.contains( "NRCKDISZ" ))
    {
        std::vector<float> &values = (_data_arrays.get_array( "NRCKDISZ" ));
        copy( begin( values ), end( values ), begin( nodal_values ) );
    }
    else if(_data_arrays.contains( "ROCKDISZ" ))
    {
        std::vector<float> &values = (_data_arrays.get_array( "ROCKDISZ" ));
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

    //this is the gpm part
    //debug: at this point, base in vs must be identical to base in gpm
    gpm_attribute& top = attributes.at( "TOP" );
    for(int k : IntRange( 1, nsurfaces ))
    {
        auto[vs_it1, vs_it2] = geometry.surface_range( k );
        auto[gpm_it1, _] = att_iterator::surface_range( top, k );
        std::copy( vs_it1, vs_it2, gpm_it1 );
    }

    //int counter = 0, nodes_per_layer = geometry->nodes_per_layer( );
    //for(int surface = 1; surface < nsurfaces; surface++) //from 1 because 0 is base and vs does not change the base.
    //{
    //    counter = 0;
    //    vector<float> &vs_depths = geometry->get_local_depths( surface );

    //    auto& holder = top[surface];
    //    for(int j = 0; j < nrows; j++)
    //    {
    //        for(int i = 0; i < ncols; i++)
    //        {
    //            holder( j, i ) = vs_depths[counter] + nodal_values[surface*nodes_per_layer + counter];
    //            counter += 1;
    //        }
    //    }
    //}



    return true;
}



int   gpm_visage_link::update_results( attr_lookup_type& attributes, std::string& error, int step )
{
    read_visage_results( _time_step, error );

    _mech_props_model->update_compacted_props( attributes, _sediments, _visage_options, _data_arrays );

    update_gpm_and_visage_geometris_from_visage_results( attributes, error );

    //now we should copy whatever results we need to copy from vs to gpm for display 
    bool include_top = false; 
    auto& to_copy = list_wanted_attribute_names( include_top );
    const auto&  geometry = _visage_options.geometry();
    auto[ncols, nrows, nsurfaces, total_nodes, total_elements] = geometry.get_geometry_description( );


    //copy props to gpm for display 
    for(const auto& vs_prop : to_copy)
    {
        vector<float> &values = _data_arrays[vs_prop.name];
        vector<float> *aux = nullptr ;
        auto to_gpm = att_iterator( attributes[vs_prop.name] );

        if( _data_arrays.array_size( vs_prop.name ) == geometry.total_nodes( ))
        {
            copy( values.begin( ), values.end( ), to_gpm );
        }
   
        else if(_data_arrays.array_size( vs_prop.name ) == geometry.total_elements( ))
        {
            vector<float> nodal_values = StructuredBase::elemental_to_nodal( ncols, nrows, nsurfaces, values );
            copy( nodal_values.begin( ), nodal_values.end( ), to_gpm );
        }
        else{ continue;} //empty values, not computed, etc...etc...       
    }


    return 0;
}