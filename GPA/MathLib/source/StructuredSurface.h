#ifndef STRUCTURED_SURFACE_H_
#define STRUCTURED_SURFACE_H_ 1

#include "StructuredBase.h"
#include "ArrayData.h"

#ifdef MATHLIB_EXPORTS
#define MATHLIB_API __declspec(dllexport)
#else
#define MATHLIB_API __declspec(dllimport)
#endif

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
StructuredSurface: public StructuredBase
{
public:

    virtual ~StructuredSurface( ) {  }

    StructuredSurface * operator->( ) { return this; }

    StructuredSurface( ) :StructuredBase( )
    {
    }

    StructuredSurface( const int ncols, const int nrows, fVector2 extent, const CoordinateMapping3D & map )
        :StructuredBase( ncols, nrows, 1, { extent[0],extent[1] }, map )
    {
        _zvalues.resize( ncols * nrows );
    }

    StructuredSurface( const int ncols, const int nrows, fVector2 extent, const CoordinateMapping3D & map, std::vector<float> & heights )
        :StructuredSurface( ncols, nrows, { extent[0],extent[1] }, map )
    {
        set_height_values( heights );
    }

    StructuredSurface( const StructuredSurface & s )
        :StructuredBase( s.ncols( ), s.nrows( ), 1, s.length( ), s.reference( ) )
    {
        _zvalues.resize( ncols( ) * nrows( ) );
        set_height_values( s._zvalues.data( ) );
        _data = s._data;
    }

    vector<float> operator -( const StructuredSurface & s2 ) const 
    {
    std::vector<float> dist( _zvalues.size(), 0.0f);
    transform( begin( _zvalues ), end( _zvalues ), s2.heights( ).cbegin( ),
    back_inserter( dist ), []( const float& h1, const float& h2 ) {return h1 - h2; } );
    return dist; 
    }

    void operator = ( const StructuredSurface & s )
    {
        StructuredBase::operator=( s );
        _zvalues.resize( ncols( ) * nrows( ) );
        set_height_values( s._zvalues.data( ) );
        _data = s._data;
    }

    void set_height_values( const std::vector<float> & values )
    {
        std::copy( values.begin( ), values.end( ), _zvalues.begin( ) );
    }

    void set_height_values( const float* values )
    {
        std::copy( values, values + _zvalues.size( ), _zvalues.begin( ) );
    }

    std::vector<float> &heights( ) {return _zvalues;}

    const std::vector<float>& heights( ) const { return _zvalues; }


    std::vector<float> get_local_coordinates( ) const;

    float& operator[]( int n ) { return _zvalues[n]; }

    float  operator[]( int n ) const { return _zvalues[n]; }

    float at( int  n ) const { return  _zvalues.at( n ); }

    fVector2 horizontal_spacing( ) const
    {
        return fVector2( _length[0] / (ncols( ) - 1), _length[1] / (nrows( ) - 1) );
    }

private:

    std::vector<float> _zvalues;

    ArrayData _data;
};

#endif
