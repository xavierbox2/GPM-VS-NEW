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
//obsolete
class MechPropertiesDVT : public IMechanicalPropertiesInitializer
{
public:

    virtual ~MechPropertiesDVT( ) = default;

    /*
    creates an elemental property for each property in the sediments table   +  InitialStiffness +  InitialPorosity
    InitialPorosity  comes from GPM as a sediment-volume-weighted property
    InitialStiffness comes from GPM as a sediment-volume-weighted property

    also creates:
        prevaling_sed_index
        dvt_table_index

    Both are elemental properties.  dvt_table_index is used by the API to write compaction tables.
    */
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



            options.add_table( index, sediments.at( sed_name ).stiffness_plasticity_table );
            index += 1;
        }

        auto& visage_data = data_arrays.get_or_create_array( "dvt_table_index", 0, prevailing_index.size( ) );
        visage_data.resize( prevailing_index.size( ), 0.0f );
        copy( prevailing_index.begin( ), prevailing_index.end( ), visage_data.begin( ) );

        auto& sed_index = data_arrays["prevaling_sed_index"];
        sed_index.resize( prevailing_index.size( ), 0.0f );
        copy( prevailing_index.begin( ), prevailing_index.end( ), sed_index.begin( ) );


    }




    //this actually only updates the porosity
    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& plastic_multiplier )
        override
    {
        options->use_tables( ) = true;

        update_porosity( sediments, options, data_arrays );

    }


};

//obsolete 
class MechPropertiesEffectiveMedium : public IMechanicalPropertiesInitializer
{
public:

    virtual ~MechPropertiesEffectiveMedium( ) = default;

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
        override
    {
        IMechanicalPropertiesInitializer::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );
        options->use_tables( ) = false;
    }

    virtual void update_porosity( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
    {
        IMechanicalPropertiesInitializer::update_porosity( sediments, options, data_arrays );
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
            auto& stiffness_table = sediments.at( sed_name ).stiffness_plasticity_table;

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

        if(data_arrays->id( ) != WellKnownVisageNames::FailureMode::ELASTIC) // if(!options->enforce_elastic( ))
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

class MechPropertiesPlasticityAndDepthDependency : public IMechanicalPropertiesInitializer
{
public:


    virtual ~MechPropertiesPlasticityAndDepthDependency( ) = default;

    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
        override
    {
        options->use_tables( ) = true;

        if(old_nsurf == new_nsurf) return;

        /*
        creates an elemental property for each property in the sediments table
        This is common to all property-updating models used.
        */
        IMechanicalPropertiesInitializer::update_initial_mech_props( atts, sediments, options, data_arrays, old_nsurf, new_nsurf );

        /*
        Monitors the initial state of some properties.
        This basically creates some "special" arrays with the initial properties.
        */
        std::vector<string> init_state_props
        {
        WellKnownVisageNames::ResultsArrayNames::Stiffness,
        WellKnownVisageNames::ResultsArrayNames::Porosity,
        WellKnownVisageNames::ResultsArrayNames::Cohesion
        };
        create_init_arrays( init_state_props, options, data_arrays, old_nsurf, new_nsurf );

        //*********************************************************/
        /****     lets create the compaction tables             ***/
        /****  these are the stiffness vs shear plastic strain. ***/
        //*********************************************************
        vector<string> sed_keys = {};
        //SED11,SED22,.SED1, SED3,..SEDN not in order, some missing but all the keys must have by now an entry in the data_arrays 
        for_each( cbegin( atts ), cend( atts ), [&sed_keys, key = "SED"]( const auto& att )
        {if(att.first.find( key ) != std::string::npos) sed_keys.push_back( att.first ); } );

        options->clear_tables( );
        int index = 0;
        vector<int> prevailing_index( options->geometry( )->total_elements( ), 0 );
        vector<float> max_concentration( options->geometry( )->total_elements( ), 0.0f );
        for(const string& sed_name : sed_keys) //SED1, SED5, SED23, SED2, SED3, ....
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

            //each sediment has a plasticity-stiffness table
            options.add_table( index, sediments.at( sed_name ).stiffness_plasticity_table );

            index += 1;
        }

        //this will be used by the deck writter 
        auto& dvt_table_index = data_arrays.get_or_create_array( "dvt_table_index", 0, prevailing_index.size( ) );

        //resize but presever the previous values although not strickly needed. 
        dvt_table_index.resize( prevailing_index.size( ), 0.0f );
        copy( prevailing_index.begin( ), prevailing_index.end( ), dvt_table_index.begin( ) );

        //*********************************************************/
        /****     lets create the stiffness-depth trends        ***/
        //*********************************************************
        //in  addition, each sediment has a table linking depth to stiffness. 
        //That depends on max burial depth ever achieved 
        //and current burrial depth. Initially, both are the same. 
        //We store those arrays here.
        auto [vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = options->geometry( ).get_geometry_description( );
        int nodal_offset = (vs_cols - 1) * (vs_rows - 1) * (old_nsurf > 0 ? (old_nsurf - 1) : 0);

        auto depth_now = options->geometry( )->nodal_to_elemental( options->geometry( )->get_depths_from_top( ) );//this is elemental 
        auto& zo_stored = data_arrays.get_or_create_array( WellKnownVisageNames::ResultsArrayNames::InitialDepth );
        zo_stored.resize( depth_now.size( ), 0.0f );
        std::copy( cbegin( depth_now ) + nodal_offset, cend( depth_now ), begin( zo_stored ) + nodal_offset );

        auto& zmax_stored = data_arrays.get_or_create_array( WellKnownVisageNames::ResultsArrayNames::MaximumDepth );//elemental as well 
        zmax_stored.resize( depth_now.size( ), 0.0f );
        std::copy( cbegin( depth_now ) + nodal_offset, cend( depth_now ), begin( zmax_stored ) + nodal_offset );

        //Create/update the initial stiffness if needed for the time step that is about to start 
        // 
        //          E = Eo exp( -k|z - zo| ) approx  E = Eo Table( |z-zo| )     
        //
        auto& ym = data_arrays[WellKnownVisageNames::ResultsArrayNames::Stiffness]; //elemental 
        const auto& init_ym = data_arrays["Init" + WellKnownVisageNames::ResultsArrayNames::Stiffness]; //elemental 

        for(int n = 0; n < depth_now.size( ); n++)
        {
            zmax_stored[n] = std::max<float>( zmax_stored[n], depth_now[n] );

            float delta_z = zmax_stored[n] - zo_stored[n];
            int sediment_index = prevailing_index[n];
            string sed_name = sed_keys.at( sediment_index );
            float depth_multiplier = sediments.at( sed_name ).stiffness_depth_table.get_interpolate( delta_z );

            ym[n] = init_ym[n] * depth_multiplier;
        }
    }

    void update_strength( map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
    {
    cout<<endl<<"Updating strength "<<std::endl;
     /*
     Implements C = Co + H e, C = cohesion, co = initial cohesion, h = hardening, e = plastic shear strain
     */
        if((data_arrays.contains( "PLSHEARSTRAIN" )) && (data_arrays.contains( "RESIDUALCOHESION" )))
        {
        //cout<<"We've got the two basic arrays"<<endl;
        //vector<string> names = data_arrays.array_names();
        //std::copy(  names.begin(), names.end(),  ostream_iterator<string> ( cout, ", "));
        //cout<<endl;


            const vector<float>& plshear = data_arrays.get_array( "PLSHEARSTRAIN" );
            vector<float>& cohesion = data_arrays.get_array( "COHESION" );
            const vector<float>& init_cohesion = data_arrays.get_array( "InitCOHESION" );
            const vector<float>& residual_cohesion = data_arrays.get_array( "RESIDUALCOHESION" );
            const vector<float>& hardening = data_arrays.get_array( "HARDENING" );
            const vector<float>& stiffness = data_arrays.get_array( "YOUNGMOD" );
            vector<float>& tensile = data_arrays.get_array( "TENSILE_STRENGTH" );

            

            //cout<<"All obtained from the stored data "<<std::endl; 

            float tolerance = 100.0f * std::numeric_limits<float>::epsilon( );
            cout<<"tolerance "<<tolerance<<endl; 

            for(int n = 0; n < cohesion.size( ); n++)
            {
                if(plshear[n] > tolerance)
                {
                    float new_cohesion = std::max<float>( cohesion[n] + hardening[n] * 1000.00f * stiffness[n] * plshear[n], residual_cohesion[n] );
                    float new_tensile  = std::min<float>( tensile[n] , new_cohesion );

                    if(n < 100)
                    {
                        cout << "old cohesion " << cohesion[n] << " new cohesion " << new_cohesion << std::endl;
                    }

                    cohesion[n] = new_cohesion;
                    tensile[n]  = new_tensile;
                }

                    

                   
            }

            //std::cout<<"Cohesion loss implemented"<<std::endl;
        }
    }


    //just needs to update the porosity for display purposes. VS takes care of plasticity and 
    //the initialization takes care of the stiffness-depth 
    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& global_table )
        override
    {
        options->use_tables( ) = true;

        update_porosity( sediments, options, data_arrays );

        update_strength( sediments, options, data_arrays );
    }

};


#endif


