#ifndef _VISAGE_RESULTS_H_
#define _VISAGE_RESULTS_H_ 1
#include <string>
#include <vector>
#include <map>
#include <functional>
#include <fstream>
#include <filesystem>
namespace fs = std::experimental::filesystem;

#include <vector>
#include "vs_api/FileSystemUtils.h"
#include "vs_api/EclipseReader.h"
#include "vs_api/ArrayData.h"

// This process is for demo only
// in essence it will ask for two properties, which is TOP and POR
// It will create an output property which is OFFSET
// It will then just call run_timestep, which in essence willl do nothing
// And when update_results are called, it will fill in an arbitrary offset, which will be the current cycle number

using namespace std;

class VisageResultsReader
{
public:

    int read_results( string model_name, string path, int step, const vector<string> &names, ArrayData &data );
    
    int read_results( string file_to_parse, const vector<string> &names, ArrayData &data );
  
    string get_results_file( string model_name, string path, int step );

    int  read_result( string file_to_parse, string keyword, ArrayData &data, map<string, float> *unit_converter = NULL, string new_name = "" );

    vector<string> get_key_names( string file ) const { return EclipseReader::GetKeywordNames( file ); }
    /*int  read_vertical_deformation(string file_to_parse, ArrayData &data, string new_name = "")
    {return read_result(file_to_parse, "ROCKDISZZ", data, new_name);
    }*/
};

#endif
