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


//this is like the dvt model but ym is increased according to depth.
//

class MechPropertiesDVT : public IMechanicalPropertiesInitializer
{
public:

    virtual void update_porosity( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
        override
    {
        IMechanicalPropertiesInitializer::update_porosity( atts, sediments, options, data_arrays );
        options->use_tables( ) = true;
    }

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
        override
    {
        IMechanicalPropertiesInitializer::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );

        options->use_tables( ) = true;

        //read as a visage result, when using dvt tables. This is a compacted YM  
        auto& visage = data_arrays[WellKnownVisageNames::ResultsArrayNames::Stiffness];
        
        //the initial ym was derived from sediments.
        auto& initial = data_arrays["Init" + WellKnownVisageNames::ResultsArrayNames::Stiffness];

        //keep a copy of the compacted one -maybe not needed-
        auto& compacted = data_arrays["CMP" + WellKnownVisageNames::ResultsArrayNames::Stiffness];

        //the reader with dvt tables reads a YM and updates it in the data arrays
        //since that info is ultimately passed again to visage, we overwrite it here so
        //we always pass the initial
        compacted.resize( visage.size( ) );
        copy( visage.begin( ), visage.end( ), compacted.begin( ) );
        copy( initial.begin( ), initial.end( ), visage.begin( ) );

        //each element will have a field "dvt_table_index", which will be 0,1,2,3....
        //depending on whether SED0, SED1,....is the most predominant component.
        vector<string> tmp = data_arrays.array_names( );
        vector<string> sed_keys;//find gpm_names SED1,SED2,...SEDN
        for_each( cbegin( tmp ), cend( tmp ), [&sed_keys, key = "SED"]( const auto& name )
        {if(name.find( key ) != std::string::npos) sed_keys.push_back( name ); } );

        vector<int> prevailing_index( options->geometry( )->total_elements( ), 0 );
        vector<float> max_concentration( options->geometry( )->total_elements( ), 0.0f );

        int index = 0;
        for(const string& sed_name : sed_keys)
        {
            vector<float> sed_concentration = options->geometry( )->nodal_to_elemental( data_arrays.get_array( sed_name ) );

            for(auto n : IntRange( 0, sed_concentration.size( ) ))
            {
                if(sed_concentration[n] > max_concentration[n])
                {
                    max_concentration[n] = sed_concentration[n];
                    prevailing_index[n] = index;
                }
            }

            

            options.add_table( index, sediments.at( sed_name ).compaction_table );
            index += 1;
        }

        auto& visage_data = data_arrays.get_or_create_array( "dvt_table_index", 0, prevailing_index.size( ) );
        visage_data.resize( prevailing_index.size( ), 0.0f );
        copy( prevailing_index.begin( ), prevailing_index.end( ), visage_data.begin( ) );
    
        auto& sed_index = data_arrays["prevaling_sed_index"];
        sed_index.resize( prevailing_index.size( ) );
        sed_index.resize( prevailing_index.size( ), 0.0f );
        copy( prevailing_index.begin( ), prevailing_index.end( ), sed_index.begin( ) );
        
    }

    //this actually only updates the porosity
    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& plastic_multiplier )
        override
    {
        options->use_tables( ) = true;

        update_porosity( atts, sediments, options, data_arrays );

    }


};

class MechPropertiesEffectiveMedium : public IMechanicalPropertiesInitializer
{
public:
    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
        override
    {
        IMechanicalPropertiesInitializer::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );
        options->use_tables( ) = false;
    }

    virtual void update_porosity( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
    {
        IMechanicalPropertiesInitializer::update_porosity( atts, sediments, options, data_arrays );
        options->use_tables( ) = false;
    }

    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& plastic_multiplier )
        override
    {
        options->use_tables( ) = false;

        update_porosity( atts, sediments, options, data_arrays );

        update_stiffness( atts, sediments, options, data_arrays, plastic_multiplier );
    }

    ////based on total strain
    //void update_porosity( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
    //{
    //    //the total volumetric strain: assume it is cummulative.
    //    vector<float>& exx = data_arrays.get_array( "STRAINXX" );
    //    vector<float>& eyy = data_arrays.get_array( "STRAINYY" );
    //    vector<float>& ezz = data_arrays.get_array( "STRAINZZ" );

    //    vector<float>& poro = data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Porosity );
    //    for(auto n : IntRange( 0, ezz.size( ) ))
    //    {
    //        float delta = -1.0f * (exx[n] + eyy[n] + ezz[n]);
    //        poro[n] = std::min<float>( 0.8f, std::max<float>( 0.025f, poro[n] + delta ) );
    //    }
    //}

    //based on porosity and compaction tables
    void update_stiffness( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& plastic_multiplier )
    {
        vector<string> tmp = data_arrays.array_names( );
        vector<string> sed_keys;//find gpm_names SED1,SED2,...SEDN
        for_each( cbegin( tmp ), cend( tmp ), [&sed_keys, key = "SED"]( const auto& name )
        {if(name.find( key ) != std::string::npos) sed_keys.push_back( name ); } );

        vector<float> ym_multiplier( options->geometry( )->total_elements( ), 0.0f ); //volume-weighted stiffness-porosity multiplier
        for(const string& sed_name : sed_keys)
        {
            vector<float> vs_elem_concent = options->geometry( )->nodal_to_elemental( data_arrays.get_array( sed_name ) );
            auto& stiffness_table = sediments.at( sed_name ).compaction_table;

            vector<float> avg_multiplier = stiffness_table.get_interpolate( data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Porosity ) );
            transform( begin( vs_elem_concent ), end( vs_elem_concent ), begin( avg_multiplier ), begin( avg_multiplier ),
                       []( float c, float m ) { return m * c; } );

                        //= Emult(phi,SED1)w1 + Emult(phi,SED2)w2 + ....from surfaces [0, surface old_num )
            for(auto n : IntRange( 0, ym_multiplier.size( ) ))
            {
                ym_multiplier[n] += avg_multiplier[n];
            }
        }

        //we add now a second multiplier, which weakens the rock on the basis of equivalent plastic strain.
        //so the final multiplier is the product of two multipliers
        vector<float> plastic_factor;
        vector<float>& ym = data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Stiffness );
        vector<float>& init_ym = data_arrays.get_array( "Init" + WellKnownVisageNames::ResultsArrayNames::Stiffness );

        if(!options->enforce_elastic( ))
        {
            cout << "\n\nConditions are not elastic" << endl;
            const vector<float>& eq = data_arrays.at( "EQPLSTRAIN" );
            plastic_factor = plastic_multiplier.get_interpolate( eq );

            cout << "\n\nMin plastic factor" << *std::min_element( plastic_factor.begin( ), plastic_factor.end( ) ) << endl;
            cout << "\n\nMax plastic factor" << *std::max_element( plastic_factor.begin( ), plastic_factor.end( ) ) << endl;
        }
        else
        {
            cout << "\n\nConditions ARE elastic" << endl;
            plastic_factor.resize( ym_multiplier.size( ), 1.0f );
        }

        for(auto n : IntRange( 0, ym_multiplier.size( ) ))
        {
            ym[n] = init_ym[n] * (ym_multiplier[n] * plastic_factor[n]);
        }
    }
};



class MechPropertiesAttys : public MechPropertiesDVT
{
public:

    

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
        override
    {
        if(old_nsurf == new_nsurf) return;

        //initial YM, initial porosity from sediments, etc...
        MechPropertiesDVT::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );

        //lets store a property z = zo, ie. initial thickness      
        auto [vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = options->geometry( ).get_geometry_description( );
        int offset = (vs_cols ) * (vs_rows ) * (old_nsurf > 0 ? (old_nsurf ) : 0);

        //create a zo property nodal, initial depth from top
        auto &zo = data_arrays["zo"];
        zo.resize( vs_total_nodes, -1 ); //preserve previous values, initials set to -1
        auto thickess_this_time = options->geometry()->get_depths_from_top();
        copy( thickess_this_time.begin( ) + offset, thickess_this_time.end( ), zo.begin( ) + offset );

        //create a zmax property nodal, 
        auto& zmax = data_arrays["zmax"];
        zmax.resize( vs_total_nodes, -1 ); //preserve previous values, initials set to -1
        copy( zo.begin( ) + offset, zo.end( ), zmax.begin( ) + offset );

    }

    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& global_table )
        override
    {

        //stiffness multiplier vs plasticity shear ->created during initialization of properties
        options->use_tables( ) = true;
        update_porosity( atts, sediments, options, data_arrays );

        cout<<"The table here is"<<global_table<<endl; 

        auto& zo = data_arrays["zo"];
        auto& zmax = data_arrays["zmax"];
        //zmax.resize( options->geometry()->total_nodes(), -999 );

        //update stiffness based on max burial depth according to the plastic_multiplier
        auto z_this_time = options->geometry( )->get_depths_from_top( );       
        for(int n =0; n < options->geometry()->total_nodes();n++)
        zmax[n] = std::max<float>( zmax[n], z_this_time[n]);
        
        //now modify the initial ym based on the global table prevaling_sed_index is elemental
        auto& sed_index = data_arrays["prevaling_sed_index"];

        //zmax - zo 
        vector<float> zmax_less_zo; 
        transform( begin(zmax), end(zmax), begin(zo ), back_inserter( zmax_less_zo ),[]( const auto &z1, const auto&z2){ return z1 - z2; } );
        vector<float> burial_depth_change = options->geometry()->nodal_to_elemental( zmax_less_zo  );
        /* ym = ym_init * exp( -k|z - zo| ) = ym_init * table( |z-zo| )
        */

        //data_arrays[WellKnownVisageNames::ResultsArrayNames::Stiffness] is the one that goes in the mat files !!
        auto& visage    = data_arrays[WellKnownVisageNames::ResultsArrayNames::Stiffness];
        auto& initial   = data_arrays["Init" + WellKnownVisageNames::ResultsArrayNames::Stiffness];

        int n_ele = options->geometry()->total_elements();
        vector<float> multiplier = global_table.get_interpolate( zmax_less_zo );
        for( int n = 0; n <  n_ele; n++ )
        visage[n] = initial[n] * multiplier[n];


    }

};


#endif
