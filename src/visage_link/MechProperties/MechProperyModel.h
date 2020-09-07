#ifndef MECH_PROPS_DVT_H_ 
#define MECH_PROPS_DVT_H_ 1

#include <algorithm>
#include <numeric>

#include "gpm_plugin_helpers.h"
#include "Definitions.h"
#include "ArrayData.h"
#include "VisageDeckSimulationOptions.h"
#include "AttributeIterator.h"
#include "Definitions.h"
#include "UIParamerers.h"
#include "IMechPropertyModel.h"

using namespace std;


//class MechPropertiesDVT : public IMechanicalPropertiesInitializer
//{
//public:
//
//
//    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays )
//        override
//    {
//
//    /*
//        vector<string> sed_keys = {}; //SED1,SED2,...SEDN  
//        for_each( cbegin( atts ), cend( atts ), [&sed_keys, key = "SED"]( const auto& att )
//        {if(att.first.find( key ) != std::string::npos) sed_keys.push_back( att.first ); } );
//
//        //compaction tables for each element -> the one for sediment of highest concentration there. 
//
//        options->use_tables( ) = true;
//        for(int n : IntRange( 0, sed_keys.size( ) )) options.add_table( n, sediments[sed_keys[n]].compaction_table );
//
//        vector<pair<float, int>> temp;
//        for(int n : IntRange( 0, sed_keys.size( ) ))
//        {
//            //vector<float> weight = get_values( atts.at( sed_keys[n] ), 0, new_nsurf );
//            vector<float> ele_weighted_index = options->geometry( )->nodal_to_elemental( get_values( atts.at( sed_keys[n] ), 0, new_nsurf ) );
//            if(n == 0)
//            {
//                transform( ele_weighted_index.begin( ), ele_weighted_index.end( ), back_inserter( temp ),
//                           []( float w )->pair<float, int> { return { w, 0 }; } );
//            }
//            else
//            {
//                for(int i : IntRange( 0, ele_weighted_index.size( ) ))
//                    if(ele_weighted_index[i] > temp[i].first) temp[i] = { ele_weighted_index[i], n };
//            }
//        }
//        auto& table_index = data_arrays["dvt_table_index"];//.get_or_create_array( prop );
//        table_index.resize( temp.size( ), 0 );
//        transform( temp.begin( ), temp.end( ), table_index.begin( ), []( pair<int, float> p ) { return p.first; } );
//    }
//
//    */
//};


class MechPropertiesEffectiveMedium : public IMechanicalPropertiesInitializer
{
public:

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays, int old_nsurf, int new_nsurf )
        override
    {

        IMechanicalPropertiesInitializer::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );
        options->use_tables( ) = false;
    }

    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays, const Table &plastic_multiplier  )
        override
    {
        options->use_tables( ) = false;
 
 
        update_porosity( atts, sediments, options, data_arrays );

        update_stiffness( atts, sediments, options,  data_arrays, plastic_multiplier );
        
    }
 

    //based on total strain
    void update_porosity( const attr_lookup_type& atts, map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays )
    {
        //the total volumetric strain: assume it is cummulative.
        vector<float>& exx = data_arrays.get_array( "STRAINXX" );
        vector<float>& eyy = data_arrays.get_array( "STRAINYY" );
        vector<float>& ezz = data_arrays.get_array( "STRAINZZ" );

        vector<float> & poro = data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Porosity);
        for (auto n : IntRange(0, ezz.size()))
        {
            float delta = -1.0f*(exx[n] + eyy[n] + ezz[n]);
            poro[n] = std::min<float>(0.8f, std::max<float>(0.025f, poro[n] + delta));
        }
    }

    //based on porosity and compaction tables 
    void update_stiffness( const attr_lookup_type& atts, map<string, SedimentDescription> &sediments, VisageDeckSimulationOptions &options, ArrayData &data_arrays, const Table &plastic_multiplier )
    {
        vector<string> tmp = data_arrays.array_names( );
        vector<string> sed_keys;//find gpm_names SED1,SED2,...SEDN  
        for_each( cbegin( tmp ), cend( tmp ), [&sed_keys, key = "SED"]( const auto& name )
        {if(name.find( key ) != std::string::npos) sed_keys.push_back( name ); } );

        vector<float> ym_multiplier( options->geometry()->total_elements(), 0.0f ); //volume-weighted stiffness-porosity multiplier 
        for(const string &sed_name : sed_keys)
        {
            vector<float> vs_elem_concent = options->geometry( )->nodal_to_elemental( data_arrays.get_array(sed_name)  );
            auto &stiffness_table = sediments.at( sed_name ).compaction_table;

            vector<float> avg_multiplier = stiffness_table.get_interpolate( data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Porosity ) );
            transform( begin( vs_elem_concent ), end( vs_elem_concent ), begin( avg_multiplier ), begin( avg_multiplier ),
            []( float c, float m ) { return m * c; } );

             //= Emult(phi,SED1)w1 + Emult(phi,SED2)w2 + ....from surfaces [0, surface old_num )
            for(auto n : IntRange( 0, ym_multiplier.size() ))
            {
                ym_multiplier[n] += avg_multiplier[n];
            }
        }


        //we add now a second multiplier, which weakens the rock on the basis of equivalent plastic strain. 
        //so the final multiplier is the product of two multipliers 
        vector<float> plastic_factor;
        vector<float>& ym = data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Stiffness );
        vector<float>& init_ym = data_arrays.get_array( "Init" + WellKnownVisageNames::ResultsArrayNames::Stiffness );
        
        if(!options->enforce_elastic())
        {
        const vector<float>& eq = data_arrays.at("EQPLSTRAIN");
        plastic_factor  = plastic_multiplier.get_interpolate( eq );
        }
        else
        {
            plastic_factor.resize( ym_multiplier.size(),1.0f);
        }
      
        for(auto n : IntRange( 0, ym_multiplier.size() ))
        {
           
            ym[n] = init_ym [n]  * (ym_multiplier[n]  * plastic_factor[n]);
        }
    }



};

#endif 

