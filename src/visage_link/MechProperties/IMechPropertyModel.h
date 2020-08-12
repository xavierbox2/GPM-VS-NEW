
#ifndef INTERFACE_MECH_PROPS_H_ 
#define INTERFACE_MECH_PROPS_H_ 1

#include "gpm_plugin_helpers.h"
#include "ArrayData.h"
#include "VisageDeckSimulationOptions.h"
#include "AttributeIterator.h"
#include "Definitions.h"
#include "UIParamerers.h"

using namespace std;

class IMechanicalPropertiesInitializer
{


protected:



public:

    virtual vector<float> get_values( const gpm_attribute& att, int k1 = 0, int k2 = -1 )
    {
        vector<float> nodal_values;
        for(int k : IntRange( k1, k2 ))
        {
            auto[it1, it2] = const_att_iterator::surface_range( att, k );
            std::copy( it1, it2, back_inserter( nodal_values ) );
        }
        return nodal_values;
    }

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays, int old_nsurf, int new_nsurf )
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

                data_arrays.set_array( key, weights );


                transform( begin( weights ), end( weights ), begin( weights ), [val = sediments.at( key ).properties.at( prop )]( float& v ){ return v * val; } );
                for(int n = 0; n < tot_nodes; n++)
                {
                    value[n] += weights[n];
                }
            }

            //value is the volume-weighted average of property = prop (nodal in gpm) 
            auto[vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = options->geometry( ).get_geometry_description( );
            int offset = (vs_cols - 1) * (vs_rows - 1) * (old_nsurf > 0 ? (old_nsurf - 1) : 0);

            auto& data_array = data_arrays[prop];//.get_or_create_array( prop );
            vector<float> ele_values = options->geometry( ).nodal_to_elemental( value );
            if(data_array.size( ) != vs_total_elements)
            {
                data_array.resize( vs_total_elements, 0.0f );
            }

            copy( ele_values.begin( ) + offset, ele_values.end( ), data_array.begin( ) + offset );
        }

    }

    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays ) = 0;

    ~IMechanicalPropertiesInitializer( ) {}


};


#endif 
