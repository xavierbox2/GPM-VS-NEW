#ifndef TABLE_H
#define TABLE_H 1

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <map>
#include <functional>
#include <unordered_map>

#include "TabularData.h"
#include "IDataSource.h"

using namespace std;



class
#ifdef ISDLL
    GPACOMMON_API
#endif
    Table//: public IDataSource
{
    friend ostream& operator<<( ostream& out, const Table& t );

public:

    ~Table( ) = default;

    Table( ) = default;

    Table& operator=( const Table& t ) = default;

    Table* operator->( ) { return this; }

    Table( const Table& t ) : Table( t._name, t._controller_var_name, t._dependant_name )
    {
        copy( t._value_pairs.begin( ), t._value_pairs.end( ), back_inserter( _value_pairs ) );
    }

    Table( string table_name, string controller, string dependant ) :
        _name( table_name ), _controller_var_name( controller ), _dependant_name( dependant ) {}

    Table( string table_name, string controller, string dependant, vector<float>& x_in, vector<float>& y_in ) :
        Table( table_name, controller, dependant )
    {
        set_values( x_in, y_in );
    }

    void set_values( const TabularData& tabular )
    {
        throw(" set_values(  const TabularData &tabular) Not implemented yet [Table]");
    }

    virtual void set_values( const IData* tabular )
    {
        throw("Not implemented yet, set_values[] ");
    }

    void set_values( vector<pair<float, float>>& vals );

    void set_values( vector<float>& x_in, vector<float>& y_in );

    void append_value( float x, float y ) { _value_pairs.push_back( { x,y } ); }

    void push_back( float x, float y ) { _value_pairs.push_back( { x,y } ); }

    size_t size( ) const noexcept { return (int)(_value_pairs.size( )); }

    string name( ) const noexcept { return _name; }

    string& name( ) { return _name; }

    string dependant_name( ) const noexcept { return _dependant_name; }

    string& dependant_name( ) { return _dependant_name; }

    string controller_var_name( ) const noexcept { return _controller_var_name; }

    string& controller_var_name( ) { return _controller_var_name; }


    float get_interpolate( float x ) const
    {
        return get_interpolate( vector<float>{x} ).at( 0 );
    }

    //returns the corresponding y(x) value according to the table for all the
    //values x passed in the vector as parameters.
    std::vector<float> get_interpolate( const vector<float>& x ) const;

    const vector< pair<float, float>>& value_pairs( )  const { return  _value_pairs; }

    vector< pair<float, float>>& value_pairs( ) { return  _value_pairs; }

    float xmin() const 
    { float x = _value_pairs[0].first;
      for( const auto& p : _value_pairs ) if( p.first < x ) x = p.first;
      return x;
    }

    float xmax( ) const
    {
        float x = _value_pairs[0].first;
        for(const auto& p : _value_pairs) if(p.first >x) x = p.first;
        return x;
    }

    void resample( int npoints)   
    {   if( (int)(size()) == npoints ) return;
        resample( xmin(), xmax(), npoints ); 
    }

    void resample( float xmin, float xmax, int npoints )
    {
        float delta = (xmax - xmin) / (npoints - 1);
        vector<float> xvals( _value_pairs.size( ) );
        for(int n = 0; n < npoints; n++) xvals[n] = xmin + n * delta;

        vector<float> yvals = get_interpolate( xvals );

        _value_pairs.resize( xvals.size( ) );
        for(int n = 0; n < xvals.size( ); n++)
        {
            _value_pairs[n] = { xvals[n], yvals[n] };
        }

    }

private:

    std::string _name, _controller_var_name, _dependant_name;
    vector< pair<float, float>> _value_pairs;

};

#endif
