#ifndef GPM_VS_INIT_BASE_H_
#define GPM_VS_INIT_BASE_H_ 1

#include <iostream>
#include <string>
#include <vector>
#include "VisageDeckSimulationOptions.h"
#include "gpm_plugin_description.h"

using namespace std;
class IConfiguration
{
public:


    virtual void initialize_vs_options( VisageDeckSimulationOptions &options ) = 0;

    virtual void initialize_model_extents( VisageDeckSimulationOptions &_visage_options, const gpm_plugin_api_model_definition* model_def )=0;

    virtual ~IConfiguration( ) {}

    virtual vector<string> OutputArraysNames( ) { return vector<string>( ); }
};

#endif
