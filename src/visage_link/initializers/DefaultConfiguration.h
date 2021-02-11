#ifndef GPM_VS_DEFAULT_CONFIG_H_
#define GPM_VS_DEFAULT_CONFIG_H_ 1

#include <iostream>

#include "IConfiguration.h"
#include "VisageDeckSimulationOptions.h"

class DefaultConfiguration : public IConfiguration
{
public:

    DefaultConfiguration( )
    {
    }

    virtual void initialize_vs_options( VisageDeckSimulationOptions &_visage_options ) override
    {
        _visage_options->set_boundary_condition( new StrainBoundaryCondition( 0, 0.0f ) );//x
        _visage_options->set_boundary_condition( new StrainBoundaryCondition( 1, 0.0f ) );//y
        _visage_options->set_boundary_condition( new DisplacementSurfaceBoundaryCondition( 2 ) );//z
        _visage_options->pinchout_tolerance( ) = 0.001f;
        _visage_options->path( ) = "D:\\GPMTESTS\\VISAGE_IO2";
        _visage_options->sea_water_density( ) = 1000.00;
        _visage_options.model_name( ) = "PALEOV3";
        
       
    }


    virtual void initialize_model_extents( VisageDeckSimulationOptions &_visage_options, const gpm_plugin_api_model_definition* model_def ) override
    {
        cout << "Initializing vs geometry extents" << endl;

        const float* x = model_def->x_coordinates;
        const float* y = model_def->y_coordinates;
        fVector3 axis1( x[1] - x[0], y[1] - y[0], 0.0f );
        fVector3 axis2( x[3] - x[0], y[3] - y[0], 0.0f );
        fVector3 axis3( 0.0f, 0.0f, 1.0f );
        fVector2 extent( axis1.length( ), axis2.length( ) );

        int ncols = (int)model_def->num_columns, nrows = (int)model_def->num_rows;
        CoordinateMapping3D reference( axis1.normalize( ), axis2.normalize( ), axis3, { 0.0f,0.0f,0.0f } );

        //
        //CoordinateMapping3D reference( fVector3(1.0,0.0,0.0), fVector3(0.0,1.0,0.0), axis3, { 0.0f,0.0f,0.0f } );

        _visage_options->geometry( ) = StructuredGrid( ncols, nrows, 0, extent, reference );
        cout << _visage_options->geometry( ) << std::endl;

    }



};

#endif
