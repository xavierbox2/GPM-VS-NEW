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


class gpm_visage_link
{

public:

    struct property_type 
    { std::string name; bool top_layer_only; 
    };

    gpm_visage_link( shared_ptr<IConfiguration>& config, shared_ptr<IMechanicalPropertiesInitializer> props_model );

    string get_id( ) const { return "GEOMECHANICS"; }

    gpm_visage_link* operator->( ) { return this; }


    //write a deck 

    void update_compacted_props( attr_lookup_type& attributes );

    bool update_boundary_conditions( const gpm_attribute& top );

    void increment_step( )
    {
        _time_step = _time_step < 0 ? 0 : _time_step + 1;
        _visage_options->update_step( _time_step );
    } //updates some options in visage

    StructuredGrid& geometry( ) { return _visage_options->geometry( ); }

    //initialization 

    bool process_ui( string_view json_string );

    void initialize_model_extents( const gpm_plugin_api_model_definition* model_def )
    {
        _config->initialize_model_extents( _visage_options, model_def );
    }

    //results 

    int  update_results( attr_lookup_type& attributes, std::string& error, int step = -1 );

    bool update_gpm_and_visage_geometris_from_visage_results( map<string, gpm_attribute>& attributes, string& error );

    int  run_timestep( const attr_lookup_type& attributes, std::string& error, const gpm_plugin_api_timespan& );

    int  run_visage( string mii_file );

    bool  gpm_visage_link::read_visage_results( int last_step, string& error );


    //gpm stuff 

    vector<pair<string, bool>> list_needed_attribute_names( const vector<string>& all_atts ) const;

    vector<gpm_visage_link::property_type> list_wanted_attribute_names( bool include_top = true ) const;

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

    std::tuple<int, int, int, int> node_values_count( const gpm_attribute& p, int  k = 0 ) const
    {
        const auto& geo = p.at( k );
        int size = (int)p.size( ), rows = (int)geo.num_rows( ), cols = (int)geo.num_cols( );

        return make_tuple( size * rows * cols, size, rows, cols );
    }

    tuple<int, int, int, int, int> gpm_attribute_sizes( const gpm_attribute& att ) const
    {
        return make_tuple( att[0].num_cols( ), att[0].num_rows( ), att.size( ), att[0].num_cols( ) * att[0].num_rows( ), att[0].num_cols( ) * att[0].num_rows( ) * att.size( ) );
    }
     
    bool update_sea_level( const attr_lookup_type& attributes )
    {
        //read the sea-level surface. This is a constant
        float  sea_level = attributes.at( "SEALEVEL" ).at( 0 )(0, 0);
        _visage_options->sea_level( ) = sea_level;
        return true;
    }

    bool _error;


    map<string, SedimentDescription> _sediments;

private:


    //helpers
    string get_visage_name_or_same( std::string gpm_name )
    {
        return _gpm_vs_name_map.find( gpm_name ) == _gpm_vs_name_map.end( ) ? gpm_name : _gpm_vs_name_map[gpm_name];
    }


    //data 
    std::set<string> _output_array_names;
    ArrayData _data_arrays;

    std::shared_ptr< StructuredSurface> prev_base;
    std::shared_ptr< StructuredSurface> base;
    Table _strain_function;
    gpm_plugin_api_timespan gpm_time;
    int _time_step;

    //vs api 
    VisageDeckSimulationOptions _visage_options;
    VisageDeckWritter _deck_writter;
    VisageResultsReader  _vs_results_reader;


    //injected config, ui and mech props 
    shared_ptr<IConfiguration> _config;
    shared_ptr<IMechanicalPropertiesInitializer> _mech_props_model;
    shared_ptr< UIParametersBase > _ui_params; 
    
    //units
    map<string, float> _from_visage_unit_conversion;
    map<string, float> _to_visage_unit_conversion;


    //not in use in version 3 of mech properties 
    Table _plasticity_multiplier;
    Table stiffness_mult_table;

    std::map<string, string> _gpm_vs_name_map;
     
    
    
};

#endif
