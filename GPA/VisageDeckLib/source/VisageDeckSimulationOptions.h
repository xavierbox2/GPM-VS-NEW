#ifndef VISAGE_DECK_SIM_OPTIONS_H
#define VISAGE_DECK_SIM_OPTIONS_H 1

#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

//TODO: Date in the MII is hard coded from 190, 1901m 1902....
//TODO: remove the preffix and pass a regex expression so it works with other codes.
//TODO: Enforce Capitals for commands
//TODO: Enforce no-white space anywhere in commands or hashes names
//TODO: SetOrReplaceHash("HEADER", "Ndisplacements  ", std::to_string(boundaryNodes));
//TODO:SetOrReplaceHash("HEADER", "Nconstraints  ", "0");
//TODO: if the value is a comma separated string, then each is a value this would fix the wirdness in LOADSTEP

//#include <windows.h>
#include <iostream>
#include <string>
#include <map>
#include <unordered_map>
#include <vector>
#include <list>

#include "ArrayData.h"
#include "Table.h"
#include "GeometrySize.h"
#include "StructuredGrid.h"
#include "BaseTypesSimulationOptions.h"
#include "BoundaryConditions.h"

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
VisageDeckSimulationOptions: public SimulationOptions
{
    static const int  NUMBERBOUNDARYCONDITIONS = 4;

public:

    VisageDeckSimulationOptions* operator->( ) { return this; }

    VisageDeckSimulationOptions( ) :SimulationOptions( "Default", 0 )
    {
        configure_default_echo( );

        configure_tidy_name( );

        configure_default_header( );

        configure_default_solver_section( );

        configure_loastep_increments( );

        configure_default_structured( );

        set_gravity( true );
        string year = std::to_string( 1900 + step( ) );
        string date = std::to_string( 1/*step()*/ ) + " \n01/01/" + year + " 00:00:00.000";
        set_command( "PRINTDATES", date );

        //PATCH
        sea_level( ) = -999.0f; //if negative, there will be no edge-loads.
        sea_water_density( ) = 1000.00f;
        pinchout_tolerance( ) = 0.001f;

        configure_default_restart( );

        configure_default_results( );

        for(int d = 0; d < NUMBERBOUNDARYCONDITIONS; d++)
            b_conditions[d] = NULL;//i, j, ktop, kbottom

        _enforce_elastic = false; 
    }

    ~VisageDeckSimulationOptions( )
    {
        delete_commands( );
        for(int d = 0; d < NUMBERBOUNDARYCONDITIONS; d++)
        if(b_conditions[d] != NULL) delete b_conditions[d];
    }

    bool _enforce_elastic;
    bool& enforce_elastic()  { return _enforce_elastic;}
    bool enforce_elastic( ) const { return _enforce_elastic; }


    void set_gravity( bool value ) {
        if(value)
            set_command( "GRAVITY", "0.0 1.0 0.0" );
        else
            set_command( "GRAVITY", "0.0 0.0 0.0" );
    }

    bool _accumm_disp;
    void set_cummulate_displacemnts( bool value )
    {
        _accumm_disp = value;
    }
    bool get_cummulate_displacemnts( ) const { return _accumm_disp; }

    /*bool _shift_k;
    void set_shift_k( bool value ) { _shift_k = value; }
    bool get_shift_k( ) const { return _shift_k; }*/

    float sea_level( ) const { return _sea_level; }
    float& sea_level( ) { return _sea_level; }

    float pinchout_tolerance( ) const { return _pinchout_tolerance; }
    float& pinchout_tolerance( ) { return _pinchout_tolerance; }

    bool _auto_config_plastic;
    bool auto_config_plasticity( ) const { return _auto_config_plastic; }
    bool& auto_config_plasticity( ) { return _auto_config_plastic; }

    float sea_water_density( ) const { return _sea_water_density; }
    float& sea_water_density( ) { return _sea_water_density; }

    void update_step( int new_step )
    {
        step( ) = new_step;
        update_step( );
    }
    void update_step( );

    string to_string( )
    {
        update_step( );
        string s = "";
        for(auto it = commands.begin( ); it != commands.end( ); ++it)
        {
            s += it->second->to_string( ) + "\n";
        }

        InstructionBlock end( "END" );
        s += end.to_string( );

        //s += "\n";
        //s += _geometry.to_string();
        //s += "\n";

        return s;
    }

    string to_string( int time_step ) {
        step( ) = time_step;
        return this->to_string( );
    }

    //dvt or other kind of tables.

    bool use_tables( ) const { return _use_tables; }

    void add_table( int index,  const Table &table )
    {
        _tables[index] = table;
    }

    bool& use_tables( ) { return _use_tables; }
    map<int, Table> _tables;

    void set_boundary_condition( IBoundaryCondition *b )
    {
        int dir = b->dir( );
        b_conditions[dir] = b;
    }
    IBoundaryCondition* get_boundary_condition( int dir )
    {
        return b_conditions[dir];
    }

    StructuredGrid& geometry( ) { return _geometry; }
    const StructuredGrid& geometry( ) const { return _geometry; }

    StructuredGrid _geometry;

    Structured_Geometry_Size geometry_size( )const 
    {
        return Structured_Geometry_Size( _geometry.ncols( ), _geometry.nrows( ), _geometry.nsurfaces( ) );
    }

    //ArrayData& properties( ) { return _mech_properties; }
    //ArrayData _mech_properties;


    //set(string command, bool value )
    //{
    //}


    private:

        //int _step;
        bool _is_elastic, _use_tables, _use_contraints, _use_displacements;
        float _erosion_tolerance, _sea_level, _sea_water_density, _pinchout_tolerance;

        IBoundaryCondition* b_conditions[NUMBERBOUNDARYCONDITIONS];

        void configure_tidy_name( )
        {
            set_command( "TIDY", "  " );
            set_command( "MODELNAME", _model_name );
        }

        void configure_loastep_increments( )
        {
            set_command( "INCREMENTS,S", "5     5" );
            set_command( "VISCOPLASTICMETHOD,S", "1.000000000E+000    1.000000000E-003" );
            set_command( "LOADSTEP", "0" );
        }

        void configure_default_structured( )
        {
        set_replace_instruction( "STRUCTUREDGRID", "idimension", std::to_string( 1 ) );
        set_replace_instruction( "STRUCTUREDGRID", "jdimension", std::to_string( 1 ) );
        set_replace_instruction( "STRUCTUREDGRID", "kdimension", std::to_string( 1 ) );
        set_replace_instruction( "STRUCTUREDGRID", "ordering", "gpp" );
        }

        void configure_default_header( );
        void configure_default_solver_section( );
        void configure_default_restart( );
        void configure_multistep_restart( );
        void configure_default_results( );
        void configure_default_echo( );
};

typedef VisageDeckSimulationOptions visage_deck_simulation_options;

#endif
