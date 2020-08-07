#include <iostream>
#include <string>
#include <algorithm>
#include <functional>
#include <vector>
#include "gpm_visage_results.h"

int  VisageResultsReader::read_result( string file_to_parse, string keyword, ArrayData &data, map<string, float> *unit_converter, string new_name )
{
    int ret_code = 0; //0 means we are ok.

    vector<float> values;
    try
    {
        if(EclipseReader::LoadEclipseDataArray( keyword, file_to_parse, values ))
        {
            if((unit_converter != NULL) && (unit_converter->find( keyword ) != unit_converter->end( )))
            {
                float factor = unit_converter->at( keyword );
                transform( values.begin( ), values.end( ), values.begin( ), [&factor]( float v )->float { return v * factor; } );
            }

            cout << "--values read of " << keyword << " = " << values.size( ) << endl;
            data->set_array( new_name.empty( ) ? keyword : new_name, values );
        }
    }
    catch(...)
    {
        ret_code = 1;
    }

    return ret_code;
}

int VisageResultsReader::read_results( string model_name, string path, int step, const vector<string> &names, ArrayData &data )
{
    string file_to_parse = get_results_file( model_name, path, step );

    return file_to_parse.empty( ) ? 1 : read_results( file_to_parse, names, data );
}

int VisageResultsReader::read_results( string file_to_parse, const vector<string> &names, ArrayData &data )
{
    int ret_code = 0;

    for(auto name : names)
    {
        ret_code += read_result( file_to_parse, name, data );
    }

    return ret_code;
}

string VisageResultsReader::get_results_file( string model_name, string path, int step )
{
    //get a list of all the visage results files (these are assumed
    //to be .X0001, .X0002, ..and pick the step needed, i.e. last_step
    std::string xfile = Utils::GetXFile( model_name, path, step );

    fs::path p1 = fs::path( path ) /= xfile;// should work windows + linux but fs is experimental in c++
    std::string file_to_parse = p1.generic_string( );

    return file_to_parse;
}