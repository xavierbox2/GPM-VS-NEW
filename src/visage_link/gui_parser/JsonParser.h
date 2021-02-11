#ifndef JSON_PARSER_H_
#define JSON_PARSER_H_ 1

#include <iostream>
#include <string>
#include <string_view>
#include <fstream>
#include <sstream>
#include <map>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <optional>

#include "Definitions.h"
#include "Table.h"
#include "UIParamerers.h"
#include "VisageDeckSimulationOptions.h"


#pragma warning(push, 0)
#include "rapidjson/document.h"
#include "rapidjson/writer.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/istreamwrapper.h"

#include <boost/algorithm/string/case_conv.hpp>

using namespace std;
using namespace rapidjson;

class JsonParser
{
public:

    static Table parse_table( Document& doc, Value::ConstMemberIterator& itr )
    {
        cout << "Parsing table " << itr->name.GetString( ) << endl;
        std::string cmp = itr->value.GetString( );
        int table_index = stoi( cmp.substr( cmp.find_last_of( '/' ) + 1, cmp.size( ) ) );
        std::string field_name = cmp.substr( cmp.find_first_of( '/' ) + 1, cmp.find_last_of( '/' ) - 1 );

        //compaction table
        const Value& table = doc[field_name].GetArray( )[table_index]["VALUES"];
        const Value& x = table[0];
        const Value& y = table[1];
        cout << "size " << x.Size( ) << endl;                                 // porosity e_Mult
        Table t( doc[field_name].GetArray( )[table_index]["NAME"].GetString( ), "strain", "time" );//plastic_shear_strain
        for(size_t n = 0; n < x.Size( ); n++)
            t.push_back( y[n].GetFloat( ), x[n].GetFloat( ) );

        return t;
    }


    template<typename T>
    static std::optional<T> parse_json_string( std::string_view json, VisageDeckSimulationOptions& visageOptions, set<std::string>& output_array_names )
    {
        return null;
    }

    static bool check_document( Document& doc )
    {

        if(doc.HasParseError( )) { cout << "There is an error in the json" << endl; return false;}
        if(doc.HasParseError( )) { cout << "Input std::string cannot be parsed"<<endl; return false;}
        if(!doc.HasMember( "SED_SOURCE" )) { cout<<"Missing SED_SOURCE"<<endl; return false;}
        if(!doc.HasMember( "PARAMETERS" )) { cout<<"Missing PARAMETERS"<<endl; return false;}

        const Value& params = doc["PARAMETERS"];
        if(!params.HasMember( "SedimentComposition" )) { cout<<"Missing SedimentComposition"<<endl; return false; }

        return true;
    }

    template<>
    static std::optional<UIParametersDVTDepthMultipliers> parse_json_string( std::string_view json, VisageDeckSimulationOptions& visageOptions, set<std::string>& output_array_names )
    {
        cout << "-----------------------------------------------------------------------------------" << endl;
        cout << "Parsing a Version UIParametersDVTDepthMultipliers input  " << endl << json << endl;
        cout<<"-----------------------------------------------------------------------------------"<<endl;

        Document doc;
        doc.Parse( json.data( ) );
        if(!check_document(doc))
        {
        return nullopt;
        }

//        visageOptions.enforce_elastic( ) = false;

        std::cout << json << std::endl;

        UIParametersDVTDepthMultipliers ui_params;
        static const char* kTypeNames[] = { "Null", "bool", "bool", "object", "Array", "std::string", "number" };
        const Value& params = doc["PARAMETERS"];
        set<std::string> results_keywords = WellKnownVisageNames::ResultsArrayNames::All( );
        set<std::string> input_keywords   = WellKnownVisageNames::VisageInputKeywords::SolverKeywords( );

        for(Value::ConstMemberIterator itr = params.MemberBegin( ); itr != params.MemberEnd( ); ++itr)
        {
            std::string name = itr->name.GetString( );

            if(kTypeNames[itr->value.GetType( )] == "bool")
            {
                bool  value = itr->value.GetBool( );
                ui_params.flags[name] = value;

                std::stringstream stream( name );
                std::for_each( istream_iterator<std::string>( stream ), istream_iterator<std::string>( ),
                               [value, &results_keywords, &output_array_names, &visageOptions]( std::string word )
                               {
                                   //if(word == "enforce_elastic") visageOptions.enforce_elastic( ) = value;

                                   //else 
                                   if(word == "manual_plastic_config") { visageOptions.auto_config_solver( ) = !value; }

                                   else if(find( results_keywords.begin( ), results_keywords.end( ), word ) != results_keywords.end( ) &&  value )
                                   output_array_names.insert( word );
                                   
                                   else{
                                   }
                               } );
            }

            else if(kTypeNames[itr->value.GetType( )] == "number")
            {
                float  value = itr->value.GetFloat( );
                ui_params.properties[name] = value;

                if(find( input_keywords.begin( ), input_keywords.end( ), name ) != input_keywords.end( ))
                {
                    visageOptions.set_value( name, std::to_string( value ) );
                }
            }


            else  if(kTypeNames[itr->value.GetType( )] == "std::string")
            {
                if(strcmp( name.c_str( ), "MaterialModel" ) == 0)
                {
                    std::string value = itr->value.GetString( );
                    std::transform( value.begin( ), value.end( ), value.begin( ), []( char c ) { return std::tolower( c ); } );
                    if(value.find( "mohr", 0 )!= std::string::npos) ui_params.properties["FAILUREMODE"] = WellKnownVisageNames::FailureMode::MOHRCOULOMB;
                    else if(value.find( "cap", 0 )!= std::string::npos) ui_params.properties["FAILUREMODE"] = WellKnownVisageNames::FailureMode::CAPROCK;
                    else
                    {
                        ui_params.properties["FAILUREMODE"] = WellKnownVisageNames::FailureMode::ELASTIC;
                        //visageOptions.enforce_elastic( ) = true;
                    }
                }

                if(strcmp( name.c_str( ), "LateralStrain" ) == 0)
                    ui_params.strain_function = parse_table( doc, itr );
            }
        }

        std::map<std::string, SedimentDescription> sediments = parse_sediments_sourceV2( doc );
        if(ui_params.properties.find( "RESIDUALCOHESION" ) != ui_params.properties.end( ))
        {
            float value = ui_params.properties["RESIDUALCOHESION"];

            for(auto it = sediments.begin( ); it != sediments.end( ); it++)
            {
                SedimentDescription& second = it->second;
                auto& props = second.properties;
                props["RESIDUALCOHESION"] = value;
            }
        }


        ui_params.sediments = sediments;

        cout << "These are the compaction tables " << endl;
        for(auto& s : sediments)
        {
            cout << s.second.stiffness_plasticity_table << endl;
        }


        return optional<UIParametersDVTDepthMultipliers>( ui_params );

    }









    /*
    Gets from the ui all the parameters identified as a known visage inoput keyword.
    Also parses the sediment description and copies into the output_array_names all the
    identified names. Whatever else, is returned in a map<std::string,strin>
    */
    /*
    template<typename T = UIParameters>
    static optional<T> parse_json_std::string( std::string_view json, VisageDeckSimulationOptions& visageOptions, set<std::string>& output_array_names )//, Table &plasticity, Table& strain_function )
    {
        auto start = chrono::steady_clock::now( );
        T ui_params;
        static const char* kTypeNames[] = { "Null", "bool", "bool", "object", "Array", "std::string", "number" };

        Document doc;
        doc.Parse( json.data( ) );

        if(doc.HasParseError( )) { throw("Input std::string cannot be parsed"); }
        if(!doc.HasMember( "SED_SOURCE" )) { throw("Missing SED_SOURCE"); }
        if(!doc.HasMember( "PARAMETERS" )) { throw("Missing PARAMETERS"); }

        const Value& params = doc["PARAMETERS"];
        if(!params.HasMember( "SedimentComposition" )) { throw("Missing SedimentComposition"); }

        set<std::string> results_keywords = WellKnownVisageNames::ResultsArrayNames::All( );
        set<std::string> input_keywords = WellKnownVisageNames::VisageInputKeywords::SolverKeywords( );
        for(Value::ConstMemberIterator itr = params.MemberBegin( ); itr != params.MemberEnd( ); ++itr)
        {
            std::string name = itr->name.GetString( );
            if(strcmp( itr->name.GetString( ), "WEAKENINGFACTOR" ) == 0)
            {
                ui_params.plasticity_multiplier = parse_table( doc, itr );
            }
            else if(strcmp( itr->name.GetString( ), "LateralStrain" ) == 0)
            {
                ui_params.strain_function = parse_table( doc, itr );
            }

            else if(kTypeNames[itr->value.GetType( )] == "bool")
            {
                bool  value = itr->value.GetBool( );
                istd::stringstream stream( name );
                std::for_each( istream_iterator<std::string>( stream ), istream_iterator<std::string>( ), [value, &results_keywords, &output_array_names, &visageOptions]( std::string word )
                               {
                                   if(word == "enforce_elastic")
                                   {
                                       visageOptions.enforce_elastic( ) = value;
                                   }
                                   else if(word == "automatic_plastic_config")
                                   {
                                       visageOptions.auto_config_plasticity( ) = !value;
                                   }
                                   else if(find( results_keywords.begin( ), results_keywords.end( ), word ) != results_keywords.end( ))
                                   {
                                       if(value)
                                           output_array_names.insert( word );
                                   }
                                   else
                                   {
                                       cout << "Unknown flag " << word << " " << value << endl;
                                   }
                               } );
            }

            else if(kTypeNames[itr->value.GetType( )] == "number")
            {

                float  value = itr->value.GetFloat( );
                if(find( input_keywords.begin( ), input_keywords.end( ), name ) != input_keywords.end( ))
                {
                    visageOptions.set_value( name, std::to_string( value ) );
                    cout << "Set visage config keyword " << name << " to " << value << endl;
                }
                else ui_params.properties[name] = value;
            }

            else  if(kTypeNames[itr->value.GetType( )] == "std::string")
            {
                ui_params.names[name] = itr->value.GetString( );
            }
        }


        std::map<std::string, SedimentDescription> sediments = parse_sediments_source( doc );

        if(ui_params.properties.find( "RESIDUALCOHESION" ) != ui_params.properties.end( ))
        {
            float value = ui_params.properties["RESIDUALCOHESION"];

            for(auto it = sediments.begin( ); it != sediments.end( ); it++)
            {
                SedimentDescription& second = it->second;
                auto& props = second.properties;
                props["RESIDUALCOHESION"] = value;
            }
        }


        ui_params.sediments = sediments;

        auto end = chrono::steady_clock::now( );

        auto  duration = chrono::duration_cast<chrono::milliseconds>(end - start).count( );
        cout << "Input file parsed in time: " << duration << " miliseconds" << endl;
        cout << "These are the compaction tables " << endl;
        for(auto& s : sediments)
        {
            cout << s.second.plasticity_compaction_table << endl;
        }


        return optional<T>( ui_params );
    }
    */

    static set<std::string>  lower_copy( const set<std::string>& c )
    {
        set<std::string> ret;
        for(const std::string& word : c)
            ret.insert( boost::to_lower_copy( word ) );
        return ret;
    }

    static std::map<std::string, SedimentDescription> parse_sediments_source( Document& doc )
    {
        const auto& seds_array = doc["SED_SOURCE"].GetArray( );
        std::map<std::string, SedimentDescription> sediments;

        for(size_t n = 0; n < seds_array.Size( ); n++)
        {
            SedimentDescription sed;
            sed.index = n;
            const auto& idparm = seds_array[n]["SEDIMENT_ID"];
            sed.id = idparm.GetString( );

            cout << "Sediment index " << n << endl;
            const auto& params = seds_array[n]["PARAMETERS"];
            for(Value::ConstMemberIterator itr = params.MemberBegin( ); itr != params.MemberEnd( ); ++itr)
            {
                if(itr->value.GetType( ) == Type::kNumberType)
                    sed.properties[itr->name.GetString( )] = itr->value.GetFloat( );

                else if((strcmp( itr->name.GetString( ), "StiffnessMultiplier" ) == 0) || (strcmp( itr->name.GetString( ), "StiffnessMultiplier" ) == 0))
                {
                    cout << "Parsing table " << itr->name.GetString( ) << endl;
                    std::string cmp = itr->value.GetString( );
                    int table_index = stoi( cmp.substr( cmp.find_last_of( '/' ) + 1, cmp.size( ) ) );
                    std::string field_name = cmp.substr( cmp.find_first_of( '/' ) + 1, cmp.find_last_of( '/' ) - 1 );

                    //compaction table
                    const Value& table = doc[field_name].GetArray( )[table_index]["VALUES"];
                    const Value& x = table[0];
                    const Value& y = table[1];
                    cout << "size " << x.Size( ) << endl;                                 // porosity e_Mult
                    //Table t( doc[field_name].GetArray( )[table_index]["NAME"].GetString( ), "porosity", "e_Mult" );


                    Table t( doc[field_name].GetArray( )[table_index]["NAME"].GetString( ), "plastic_shear_strain", "e_Mult" );




                    for(size_t n = 0; n < x.Size( ); n++)
                        t.push_back( y[n].GetFloat( ), x[n].GetFloat( ) );
                    sed.stiffness_plasticity_table = t;
                }

                else { ; }
            }

            sediments["SED" + std::to_string( n + 1 )] = sed;
        }


        cout << "Compaction read for sed 1" << endl;
        cout << sediments["SED1"].stiffness_plasticity_table << endl;


        for(int n = 1; n < 10; n++)
        {
            std::string fake_name = "SED" + std::to_string( n );
            if(sediments.find( fake_name ) == sediments.end( ))
            {
                cout << "Creating fake sediment " << fake_name << endl;
                sediments[fake_name] = sediments["SED1"];
                sediments[fake_name].index = n;
            }

        }


        return sediments;
    }

    template<typename Itr>
    static std::string get_value( Itr& i )
    {
        static  auto f0 = []( Itr& itr ) -> std::string { return "NULL"; };
        static  auto f1 = []( Itr& itr ) -> std::string { return std::to_string( itr->value.GetBool( ) ); };
        static  auto f2 = []( Itr& itr ) -> std::string { return std::to_string( itr->value.GetBool( ) ); };
        static  auto f3 = []( Itr& itr ) -> std::string { return  "object"; };
        static  auto f4 = []( Itr& itr ) -> std::string { return "object"; };
        static  auto f5 = []( Itr& itr ) -> std::string { return itr->value.GetString( ); };
        static  auto f6 = []( Itr& itr ) -> std::string { return std::to_string( itr->value.GetFloat( ) ); };

        static  map<int, function<std::string( Value::ConstMemberIterator& )>> functions =
        { {0,f0},{1,f1},{2,f2},{3,f3},{4,f4},{5,f5},{6,f6}
        };

        return functions[(int)(i->value.GetType( ))]( i );
    };



    static std::map<std::string, SedimentDescription> parse_sediments_sourceV2( Document& doc )
    {
        const auto& seds_array = doc["SED_SOURCE"].GetArray( );
        std::map<std::string, SedimentDescription> sediments;

        for(size_t n = 0; n < seds_array.Size( ); n++)
        {
            SedimentDescription sed;
            sed.index = n;
            const auto& idparm = seds_array[n]["SEDIMENT_ID"];
            sed.id = idparm.GetString( );

            cout << "Sediment index " << n << endl;
            const auto& params = seds_array[n]["PARAMETERS"];
            for(Value::ConstMemberIterator itr = params.MemberBegin( ); itr != params.MemberEnd( ); ++itr)
            {
                if(itr->value.GetType( ) == Type::kNumberType)
                {
                    sed.properties[itr->name.GetString( )] = itr->value.GetFloat( );
                }
                else if(strcmp( itr->name.GetString( ), "StiffnessPlasticityMultiplier" ) == 0)
                {
                    cout << "Parsing table " << itr->name.GetString( ) << endl;
                    sed.stiffness_plasticity_table = parse_table( doc, itr );
                    sed.stiffness_plasticity_table.controller_var_name( ) = "plastic_shear_strain";
                    sed.stiffness_plasticity_table.dependant_name( ) = "e_Mult";
                }

                else if(strcmp( itr->name.GetString( ), "StiffnessDepthMultiplier" ) == 0)
                {
                    cout << "Parsing table " << itr->name.GetString( ) << endl;
                    sed.stiffness_depth_table = parse_table( doc, itr );
                    sed.stiffness_depth_table.controller_var_name( ) = "max_depth";
                    sed.stiffness_depth_table.dependant_name( ) = "e_Mult";
                }

                else { ; }
            }

            sediments[sed.id] = sed;
            //sediments["SED" + std::to_string( n + 1 )] = sed;
        }


        //cout << "Compaction tables read for SED1" << endl;
        //cout << sediments["SED1"].plasticity_compaction_table << endl;
        //cout << sediments["SED1"].depth_compaction_table << endl;


        //for(int n = 1; n < 10; n++)
        //{
        //    std::string fake_name = "SED" + std::to_string( n );
        //    if(sediments.find( fake_name ) == sediments.end( ))
        //    {
        //        cout << "Creating fake sediment " << fake_name << endl;
        //        sediments[fake_name] = sediments["SED1"];
        //        sediments[fake_name].index = n;
        //    }

        //}


        return sediments;
    }






};

#pragma warning(pop)

#endif
