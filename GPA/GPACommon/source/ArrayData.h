#ifndef ARRAY_DATA_H
#define ARRAY_DATA_H 1

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <numeric>
#include <map>
#include <set>
#include <functional>
#include <unordered_map>

using namespace std;

#ifdef GPACOMMON_EXPORTS
#define GPACOMMON_API __declspec(dllexport)
#else
#define GPACOMMON_API __declspec(dllimport)
#endif

class
#ifdef ISDLL
    GPACOMMON_API
#endif
    ArrayData
{
    using iterator = std::unordered_map<std::string, std::vector<float>>::iterator;
    using c_iterator = std::unordered_map<std::string, std::vector<float>>::const_iterator;

public:

    ArrayData( ) = default;

    ArrayData( const ArrayData &idata )
    {
        for(c_iterator it = idata.arrays.begin( ); it != idata.arrays.end( ); ++it)
        {
            set_array( it->first, it->second );
        }
    }

    ArrayData( ArrayData &&idata ) noexcept
    {
        arrays = std::move( idata.arrays );
        idata.reset( );
    }

    ArrayData* operator->( ) { return this; }

    ArrayData& operator=( const ArrayData &idata );

    ArrayData& operator=( ArrayData &&idata ) noexcept
    {
        if(&idata != this)
        {
            arrays = std::move( idata.arrays );
            idata.reset( );
        }

        return *this;
    }

    iterator begin( )
    {
        return arrays.begin( );
    }

    iterator end( )
    {
        return arrays.end( );
    }

    c_iterator cbegin( )
    {
        return arrays.cbegin( );
    }

    c_iterator cend( )
    {
        return arrays.cend( );
    }

    ArrayData*  set_array( std::string name, const float *values, int count );

    ArrayData*  set_array( std::string name, const std::vector<float> &values );

    ArrayData*  set_array( std::string name, float single_value = 0.0f, int size = 1 );

    std::vector<float>& get_array( std::string name );

    std::vector<float>& get_or_create_array( std::string name, float default_value = 0.0f, int default_size = 1 );

    float get_value_or_default( string name, int index ) const
    {
        return arrays.at( name ).at( std::min<int>( index, (int)(arrays.at( name ).size( )) - 1 ) );
    }

    float get_value( string name, int index ) const
    {
        return arrays.at( name ).at( index );
    }

    float vdef( string name, int index ) const
    {
        return arrays.at( name ).at( std::min<int>( index, (int)(arrays.at( name ).size( )) - 1 ) );
    }

    size_t delete_array( std::string name )
    {
        return arrays.erase( name );
    }

    void clear( ) noexcept
    {
        arrays.clear( );
    }

    void reset( ) noexcept
    {
        arrays.clear( );
    }

    bool contains( std::string name ) const noexcept
    {
        return arrays.find( name ) != arrays.end( );
    }

    bool empty( ) const noexcept { return arrays.empty( ); }

    int count( ) const noexcept { return static_cast<int>(arrays.size( )); }

    void set_default_elastic( )
    {
        set_array( "YOUNGMOD", 5.00e7f );
        set_array( "POISSONR", 0.3f );
        set_array( "DENSITY", 2700.0f );// 400.144f);
        set_array( "BIOTC", 1.0f );
        set_array( "POROSITY", 0.0f );
    }

    void set_default_plastic( )
    {
        set_default_elastic( );
        set_array( "COHESION", 50.0f );
    }

    const vector<float>& operator[]( string s ) const { return arrays.at( s ); }

    const vector<float>& at( string s ) const { return arrays.at( s ); }

    vector<float>& operator[]( string s ) { return arrays[s]; }

    vector<string> array_names( ) const noexcept
    {
        vector<string> ret;
        for(const auto pair : arrays) ret.push_back( pair.first );
        return ret;
    }

    size_t array_size( string name  ) const noexcept
    {
         return contains(name)? arrays.at(name).size() : 0; 
    }
 
private:

    std::unordered_map<std::string, std::vector<float>>  arrays;
};

#endif
