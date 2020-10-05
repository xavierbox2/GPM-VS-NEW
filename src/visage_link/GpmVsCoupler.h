#ifndef _VISAGE_LINK_H_
#define _VISAGE_LINK_H_ 1
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <filesystem>
#include <iterator>
#include <algorithm>
#include <numeric>
namespace fs = std::experimental::filesystem;

#include <vector>
#include <iterator>
#include <set>
#include <chrono>

#include "utils.h"
#include "AttributeIterator.h"
#include "IConfiguration.h"
#include "DefaultConfiguration.h"
#include "JsonParser.h"
#include "UIParamerers.h"

#include "Definitions.h"
#include "Vector3.h"
#include "FileSystemUtils.h"

#include "gpm_plugin_helpers.h"
#include "gpm_plugin_description.h"
#include "gpm_visage_sed_description.h"


#include "BaseTypesSimulationOptions.h"
#include "VisageDeckSimulationOptions.h"
#include "VisageDeckWritter.h"
#include "VisageDeckSimulationOptions.h"
#include "VisageDeckWritter.h"
#include "IMechPropertyModel.h"
#include "MechProperyModel.h"
#include "gpm_visage_results.h"



using namespace std;
class ChronoPoint
{
public:

    ChronoPoint( string s = " " )
    {
        name = s;
        start = std::chrono::system_clock::now( );
    }

    ~ChronoPoint( )
    {
        std::chrono::system_clock::time_point end = std::chrono::system_clock::now( );
        std::cout << "********Elapsed time " << name
            << " (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count( )
            << " (us): " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count( )
            << endl;
    }

    string name; std::chrono::system_clock::time_point start;
};


class gpm_visage_link
{
//temporal. make it as an injected dependency 





    struct property_type { std::string name; bool top_layer_only; };

    std::set<string> _output_array_names;
    VisageDeckSimulationOptions _visage_options;
    VisageDeckWritter _deck_writter;
    VisageResultsReader  _vs_results_reader;
    ArrayData _data_arrays;
    map<string, SedimentDescription> _sediments;

    shared_ptr<IConfiguration> _config;
    shared_ptr<IMechanicalPropertiesInitializer> _mech_props_model;
    map<string, float> _from_visage_unit_conversion;
    map<string, float> _to_visage_unit_conversion;

    Table _plasticity_multiplier, _strain_function;
    gpm_plugin_api_timespan gpm_time;


    //float _lateral_strain;
    int _time_step;

public:

    gpm_visage_link( shared_ptr<IConfiguration>& config, shared_ptr<IMechanicalPropertiesInitializer> props_model )
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
         { WellKnownVisageNames::ResultsArrayNames::Stiffness, 1000000.00f}, //all the program is in GPa ->passed as KPa to vs
         {"COHESION", 1000.00f}, //all the program is in MPa ->passed as KPa to vs
         {"TENSILE_STRENGTH", 1000.00f}, //all the program is in MPa ->passed as KPa to vs
         {"DENSITY", 10.00f} //all the program is in gr/cm3 ->passed as KPa rho * g 
        };




        _plasticity_multiplier.append_value( 0.0, 1.0 );
        _plasticity_multiplier.append_value( 1.0, 1.0 );


    }

    gpm_visage_link* operator->( ) { return this; }

    void update_compacted_props( attr_lookup_type& attributes );

    tuple<int, int, int, int, int> gpm_attribute_sizes( const gpm_attribute& att ) const
    {
        return make_tuple( att[0].num_cols( ), att[0].num_rows( ), att.size( ), att[0].num_cols( ) * att[0].num_rows( ), att[0].num_cols( ) * att[0].num_rows( ) * att.size( ) );
    }

    string write_deck( )
    {
        string s = VisageDeckWritter::write_deck( &_visage_options, &_data_arrays );
        return s;
    }

    bool update_boundary_conditions( const gpm_attribute& top );

    void increment_step( )
    {
        _time_step = _time_step < 0 ? 0 : _time_step + 1;
        _visage_options->update_step( _time_step );
    } //updates some options in visage

    bool process_ui( string_view json_string );

    void initialize_model_extents( const gpm_plugin_api_model_definition* model_def )
    {
        _config->initialize_model_extents( _visage_options, model_def );
    }

    vector<pair<string, bool>> list_needed_attribute_names( const vector<string>& all_atts ) const;

    vector<gpm_visage_link::property_type> list_wanted_attribute_names( bool include_top = true ) const;

    int  update_results( attr_lookup_type& attributes, std::string& error, int step = -1 );


    bool update_gpm_and_visage_geometris_from_visage_results( map<string, gpm_attribute>& attributes, string& error );

    int  run_timestep( const attr_lookup_type& attributes, std::string& error, const gpm_plugin_api_timespan& );

    vector<float> get_values( const gpm_attribute& att, int k1 = 0, int k2 = -1 )
    {
        vector<float> nodal_values;
        for(int k : IntRange( k1, k2 ))
        {
            auto [it1, it2] = const_att_iterator::surface_range( att, k );
            std::copy( it1, it2, back_inserter( nodal_values ) );
        }
        return nodal_values;
    }

    vector<float> get_gpm_heights( const gpm_attribute& att, int k1 = 0 )
    {
        vector<float> nodal_values;
        auto [it1, it2] = const_att_iterator::surface_range( att, k1 );
        std::copy( it1, it2, back_inserter( nodal_values ) );
        return nodal_values;
    }


    bool  gpm_visage_link::read_visage_results( int last_step, string& error );

    std::tuple<int, int, int, int> node_values_count( const gpm_attribute& p, int  k = 0 ) const
    {
        const auto& geo = p.at( k );
        int size = (int)p.size( ), rows = (int)geo.num_rows( ), cols = (int)geo.num_cols( );

        return make_tuple( size * rows * cols, size, rows, cols );
    }

public:

    void update_mech_props( const attr_lookup_type& atts, int old_num_surfaces, int new_num_surfaces )
    {
        _mech_props_model->update_initial_mech_props( atts, _sediments, _visage_options, _data_arrays, old_num_surfaces, new_num_surfaces );
        _mech_props_model->update_compacted_props( atts, _sediments, _visage_options, _data_arrays, _plasticity_multiplier );
    }

    void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
    {
    }


    bool update_sea_level( const attr_lookup_type& attributes )
    {
        //read the sea-level surface. This is a constant
        float  sea_level = attributes.at( "SEALEVEL" ).at( 0 )(0, 0);
        _visage_options->sea_level( ) = sea_level;
        return true;
    }

    StructuredGrid& geometry( ) { return _visage_options->geometry( ); }

    int  run_visage( string mii_file );

    bool _error;


private:

    bool copy_visage_to_attribute( gpm_attribute& prop, const string& name, string& error )
    {
        //debug

        auto [ncols, nrows, nsurfaces, total_nodes, total_elements] = _visage_options->geometry( )->get_geometry_description( );
        std::vector<float> nodal_values;
        {
            if(!_data_arrays.contains( name ))
            {
                cout << "[copy_visage_to_attribute] visage array " << name << " not found when copying ts values to gpm" << endl;
                return false;
            }

            vector<float>& values = _data_arrays.get_array( name );
            nodal_values.resize( total_nodes );

            if(values.size( ) == total_elements)
            {
                //We need to pass them to gpm as nodal properties.
                nodal_values = StructuredGrid::elemental_to_nodal( ncols, nrows, nsurfaces, values );//, values);
            }
            else if(values.size( ) == total_nodes) //inneficient !!
            {
                copy( begin( values ), end( values ), begin( nodal_values ) );
            }
            else if(values.size( ) == 1) //constant !!
            {
                nodal_values.resize( total_nodes, values[0] );
            }

            else
            {
                error += "\n[update_gpm_geometry_from_visage] The number of values read does not match elements or nodes when reading X files";
                return false;
            }
        }

        int counter = 0, offset = 0;
        for(int surface = 0; surface < nsurfaces; surface++)
        {
            auto& my_array_holder = prop.at( surface );
            for(int j = 0; j < nrows; j++)
            {
                for(int i = 0; i < ncols; i++)
                {
                    //prop[surface](j, i) = nodal_values[offset + counter];
                    my_array_holder( j, i ) = nodal_values[offset + counter];
                    counter += 1;
                }
            }
        }

        return true;
    }
    //helpers
    string get_visage_name_or_same( std::string gpm_name )
    {
        return _gpm_vs_name_map.find( gpm_name ) == _gpm_vs_name_map.end( ) ? gpm_name : _gpm_vs_name_map[gpm_name];
    }

    vector<float> get_gpm_difference( const gpm_attribute& top, int k1, int k2 )
    {
        int counter = 0;
        vector<float> diff( top[k1].num_rows( ) * top[k1].num_cols( ), 0.0f );
        //diff.resize(top[k1].num_rows() * top[k1].num_cols(), 0.0f);

        for(int row = 0; row < top[k1].num_rows( ); row++)
        {
            for(int col = 0; col < top[k1].num_cols( ); col++)
            {
                diff[counter++] = top[k1]( row, col ) - top[k2]( row, col );
            }
        }

        return diff;
    }

    vector<float> get_gpm_height( const vector<Slb::Exploration::Gpm::Api::array_2d_indexer<float>>& top, int k1 )
    {
        /*
         auto [i1,i2] = att_iterator<gpm_attribute>::range(atts["TOP"][k1]);
         copy( i1,i2, back_inserted( heights));

         for_range( i1,i2, [ &height]( const float v ){  };);
        */

        int counter = 0;
        vector<float> height( top[k1].num_rows( ) * top[k1].num_cols( ), 0.0f );

        for(int row = 0; row < top[k1].num_rows( ); row++)
        {
            for(int col = 0; col < top[k1].num_cols( ); col++)
            {
                height[counter++] = top[k1]( row, col );
            }
        }

        return height;
    }

    std::map<string, string> _gpm_vs_name_map;
    std::shared_ptr< StructuredSurface> prev_base;
    std::shared_ptr< StructuredSurface> base;


    //experimental. Temporal
    Table stiffness_mult_table;
    vector<float> gpm_below, gpm_above, last_known_layer;


};

#endif
