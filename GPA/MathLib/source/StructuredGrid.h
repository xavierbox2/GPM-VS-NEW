#ifndef GEOMETRY_H_
#define GEOMETRY_H_ 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <vector>
#include <map>
#include <string>
#include <tuple>

#include <algorithm>
#include <numeric>

#include "Vector3.h"
#include "Range.h"
#include "CoordinateMapping.h"
#include "StructuredSurface.h"
#include "GridGeometry.h"

#ifdef MATHLIB_EXPORTS
#define MATHLIB_API __declspec(dllexport)
#else
#define MATHLIB_API __declspec(dllimport)
#endif

using namespace std;

/*
This type represents a structured and regular in XY grid. the grid consists in a
number of surfaces + the horizontal definition of sizes. This grid spacing can
be different in x and Y but it is constant along each direction. The spacing in
z can be heterogeneous. Basically, this class is the base class + all the functionality
to store/modify/query non-uniform z coordinates of the nodes.

This is the kind of grid that matches gpm data structure
*/



class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
StructuredGrid: public StructuredBase, public IGridGeometry
    {
    public:

    virtual~StructuredGrid( ) {}

    StructuredGrid * operator->( ) { return this; }

    StructuredGrid( ) = default;

    StructuredGrid( const int ncols, const int nrows, const int nlayers, fVector2 extent, const CoordinateMapping3D & map );

    StructuredGrid& operator = ( StructuredGrid & g );

    //<x1,y1,z1, <x2,y2,z2>, ...for surface
    vector<fVector3> get_local_coordinates_vector( int surface_index ) const;

    //x1,y1,z1, x2,y2,z2, ...
    std::vector<float> get_local_coordinates( ) const;

    //x1,y1,z1, x2,y2,z2, ...for surface
    std::vector<float> get_local_coordinates( int surface_index ) const;

    std::vector<float>::iterator surface_height_begin( int k ) { return _zvalues[k].begin( ); }

    std::vector<float>::iterator surface_height_end( int k ) { return _zvalues[k].end( ); }

    tuple< vector<float>::iterator, vector<float>::iterator> surface_height_begin_end( int k )
    { return make_tuple( surface_height_begin( k ), surface_height_end( k ) );
    }

    vector<float>& get_heights( int surface_index );
    
    void set_z_values( int surface_index, std::vector<float> & values )
    {
    add_surfaces_if_needed( surface_index );
    std::copy( values.begin( ), values.end( ), _zvalues[surface_index].begin( ) );
    }

    void set_z_values( int surface_index,float value )
    {
    add_surfaces_if_needed( surface_index );
    for(auto& v : _zvalues[surface_index]) v = value;
    }

    void set_z_values( int surface_index, float* values )
    {
    add_surfaces_if_needed( surface_index );
    std::vector<float>& z = _zvalues[surface_index];
    std::copy( values, values + z.size( ), _zvalues[surface_index].begin( ) );
    }

    void displace_all_nodes( std::vector<float> & displacement );

    void set_num_surfaces( int nz )
    {
    _node_count[2] = nz;
    add_surfaces_if_needed( _node_count[2] - 1 );
    }

    std::vector<float>& get_local_depths( int nk ) { return _zvalues[nk]; }

    std::vector<float> get_local_depths( int nk ) const
    {
    return _zvalues[nk];
    }

    StructuredSurface get_structured_surface( int index )
    {
    return StructuredSurface( ncols( ), nrows( ), length( ), reference( ), _zvalues[index] );
    }

    std::vector<int> get_height_overlaps( int surface1, int surface2, float tolerance ) const;

    void brute_force_get_pinched_elements( float pinchout_tolerance, vector<int> & element_pinched_count, map<int, int> & node_connections );

    vector<float>::iterator begin_surface( int k ) { return _zvalues[k].begin( );}

    tuple< vector<float>::const_iterator, vector<float>::const_iterator> surface_range( int k1 )
    { return make_tuple( cbegin_surface(k1), cend_surface(k1));
    }


    vector<float>::const_iterator cbegin_surface( int k ) const { return _zvalues.at(k).cbegin( ); }

    vector<float>::iterator end_surface( int k ) { return _zvalues[k].end( ); }

    vector<float>::const_iterator cend_surface( int k ) const { return _zvalues.at( k ).cend( ); }


    protected:

    void add_surfaces_if_needed( int surface_index )
    {
     while(surface_index >= _zvalues.size( ))
     {
     _zvalues.push_back( std::vector<float>( nodes_per_layer( ) ) );
     _node_count[2] = (int)_zvalues.size( );
     }

    }

    std::vector<std::vector<float>> _zvalues;
    };


class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
StructuredDeformableGrid: public StructuredGrid
{
public:

virtual ~StructuredDeformableGrid( ) {}

void add_displacements( const std::vector<float> & disp, int dir )
{
throw exception( "Not implemented yet [StructuredDeformableGrid::add_displacements( const std::vector<float> &disp, int dir )] " );
}

 std::vector<float> get_local_coordinates( ) const { return vector<float>( ); }

 std::vector<float> get_local_coordinates( int surface_index ) const { return vector<float>( ); }

 void get_local_coordinates_vector( int surface_index, std::vector<fVector3> & ret ) const { ; }

 std::vector<fVector3> get_local_coordinates_vector( int surface_index ) const { return vector<fVector3>( ); }

 std::vector<int> get_height_overlaps( int surface1, int surface2, float tolerance ) const { return vector<int>( ); }

 void brute_force_get_pinched_elements( float pinchout_tolerance, vector<int> & element_pinched_count, map<int, int> & node_connections ) { ; }

void operator = ( StructuredDeformableGrid & g ) { ; }

protected:

//cummulated displacements
 std::vector<std::vector<float>> _xdisp;
 std::vector<std::vector<float>> _ydisp;
 std::vector<std::vector<float>> _zdisp;
};

#endif
