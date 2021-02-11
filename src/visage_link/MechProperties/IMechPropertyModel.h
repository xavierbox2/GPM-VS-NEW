
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


public:

    virtual ~IMechanicalPropertiesInitializer( ) = default;

    virtual vector<float> get_values( const gpm_attribute& att, int k1 = 0, int k2 = -1 )
    {
        vector<float> nodal_values;
        for(int k : IntRange( k1, k2 ))
        {
            auto [it1, it2] = const_att_iterator::surface_range( att, k );
            std::copy( it1, it2, back_inserter( nodal_values ) );
        }
        return nodal_values;
    }

    //based on total strain
    virtual void update_porosity(  map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays )
    {
        //the total volumetric strain: assume it is cummulative.
        vector<float>& exx = data_arrays.get_array( "STRAINXX" );
        vector<float>& eyy = data_arrays.get_array( "STRAINYY" );
        vector<float>& ezz = data_arrays.get_array( "STRAINZZ" );

        const vector<float>& initPoro = data_arrays.get_array( "Init" + WellKnownVisageNames::ResultsArrayNames::Porosity );
        vector<float>& poro = data_arrays.get_array( WellKnownVisageNames::ResultsArrayNames::Porosity );

        ////option 1: apparently the correct one
        for(auto n : IntRange( 0, ezz.size( ) ))
        {
            float delta = -1.0f * (exx[n] + eyy[n] + ezz[n]);
            poro[n] = std::min<float>( 0.8f, std::max<float>( 0.025f, initPoro[n] + delta ) );
        }

        //option 2: technically wrong but appears to be better ?
        //for(auto n : IntRange( 0, ezz.size( ) ))
        //{
        //    float delta = -1.0f * (exx[n] + eyy[n] + ezz[n]);
        //    poro[n] = std::min<float>( 0.8f, std::max<float>( 0.025f, poro[n] + delta ) );
        //}

    }

    //creates and populates arrays with the name "Initxx" with initial properties given to them
    virtual void create_init_arrays( const vector<string> &names, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
    {
        auto [vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = options->geometry( ).get_geometry_description( );
        int offset = (vs_cols - 1) * (vs_rows - 1) * (old_nsurf > 0 ? (old_nsurf - 1) : 0);

           //we need to keep a copy of the intial stiffness and porosity, this lambda will create them
        auto make_initial = [&data_arrays, offset]( string variable_name )
        {
            auto& intitial = data_arrays["Init" + variable_name];
            auto& values = data_arrays[variable_name];
            intitial.resize( values.size( ) );
            copy( begin( values ) + offset, end( values ), begin( intitial ) + offset );
        };

        for( std::string name : names )
        {
            make_initial( name );
        }
      

    }

    /*
    creates an elemental property for each property in the sediments table   +  InitialStiffness +  InitialPorosity
    InitialPorosity  comes from GPM as a sediment-volume-weighted property
    InitialStiffness comes from GPM as a sediment-volume-weighted property
    */
    virtual void update_initial_mech_props( const attr_lookup_type& atts, const map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, int old_nsurf, int new_nsurf )
    {
        if(old_nsurf == new_nsurf) return;
        
        vector<string> sed_keys = {}; //SED1,SED2,...SEDN  
        for_each( cbegin( atts ), cend( atts ), [&sed_keys, key = "SED"]( const auto& att )
        {if(att.first.find( key ) != std::string::npos) sed_keys.push_back( att.first ); } );
         

        set<string> prop_names = { sediments.at( sed_keys[0] ).property_names( ) }; //"POROSITY", "DENSITY", "YOUNGMOD",......etc...")
        auto [vs_cols, vs_rows, vs_surfaces, vs_total_nodes, vs_total_elements] = options->geometry( ).get_geometry_description( );
        int offset = (vs_cols - 1) * (vs_rows - 1) * (old_nsurf > 0 ? (old_nsurf - 1) : 0);
         

        int tot_nodes = (atts.at( "TOP" ).size( ) * atts.at( "TOP" )[0].num_cols( ) * atts.at( "TOP" )[0].num_rows( ));
        vector<float> value( tot_nodes );
         

        /*
        properties weighted by the volume of sediments
        */
        for(string prop : prop_names)  // for each sediment-related property: POROSITY, STIFFNESS, etc...
        {
            //get the weighted volume average 
            fill( value.begin( ), value.end( ), 0.0f );
            for(string key : sed_keys)
            {

                if(atts.find( key ) != atts.end( ))
                {
                    vector<float> weights = get_values( atts.at( key ), 0, new_nsurf );

                    data_arrays.set_array( key, weights );

                    transform( begin( weights ), end( weights ), begin( weights ), [val = sediments.at( key ).properties.at( prop )]( float& v ){ return v * val; } );
                    for(int n = 0; n < tot_nodes; n++)
                    {
                        value[n] += weights[n];
                    }
                }
            }
             
            //value is the sediment-volume-weighted average of property = prop (nodal in gpm) porosity, stiffness,etc. whatever in the outer loop
            auto& data_array = data_arrays[prop];
            vector<float> ele_values = options->geometry( ).nodal_to_elemental( value );
            if(data_array.size( ) != vs_total_elements)
            {
                data_array.resize( vs_total_elements, 0.0f ); //initial sediment-related property
            }

            copy( ele_values.begin( ) + offset, ele_values.end( ), data_array.begin( ) + offset );
        }


        ////we need to keep a copy of the intial stiffness and porosity, this lambda will create them
        //auto make_initial = [&data_arrays, offset]( string variable_name )
        //{
        //    auto& intitial = data_arrays["Init" + variable_name];
        //    auto& values = data_arrays[variable_name];
        //    intitial.resize( values.size( ) );
        //    copy( begin( values ) + offset, end( values ), begin( intitial ) + offset );
        //};

        //make_initial( WellKnownVisageNames::ResultsArrayNames::Stiffness );
        //make_initial( WellKnownVisageNames::ResultsArrayNames::Porosity );
        //make_initial( WellKnownVisageNames::ResultsArrayNames::Cohesion );
   }

    virtual void update_compacted_props( const attr_lookup_type& atts, map<string, SedimentDescription>& sediments, VisageDeckSimulationOptions& options, ArrayData& data_arrays, const Table& plastic_multiplier ) = 0;

    
 


};


#endif 
