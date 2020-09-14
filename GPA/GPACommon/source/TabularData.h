#ifndef TABULAR_DATA_H 
#define TABULAR_DATA_H 1 

#include "IDataSource.h"

class
#ifdef ISDLL
    GPACOMMON_API
#endif

TabularData: public IData
{
public:

    ~TabularData( ) = default;

    TabularData( ) = default;

    TabularData( const TabularData & t ) = default;

    TabularData& operator=( const TabularData & t ) = default;

    TabularData( TabularData && idata ) noexcept
    {
        _value_pairs = std::move( idata._value_pairs );
        idata.reset( );
    }

    TabularData* operator->( ) { return this; }

    virtual size_t size( ) const noexcept override
    {
    return _value_pairs.size( );
    }

    void reset( ) { _value_pairs.clear( ); }

    vector< pair<float, float>>& value_pairs( ) { return _value_pairs; }

    vector< pair<float, float>> value_pairs( ) const { return _value_pairs; }

    void set_values( vector<pair<float, float>> & vals )
    {
        _value_pairs = vals;
        sort( _value_pairs.begin( ), _value_pairs.end( ) );
    }

    void set_values( vector<float> & x_in, vector<float> & y_in )
    {
        _value_pairs.clear( );
        for(int n = 0; n < (int)x_in.size( ); n++)
        {
            _value_pairs.push_back( make_pair( x_in[n], y_in[n] ) );
        }

        sort( _value_pairs.begin( ), _value_pairs.end( ) );
    }


    float get_interpolate(  float x ) const
    {
        return get_interpolate(  vector<float>{x}  ).at(0);
    }

    std::vector<float> get_interpolate( const vector<float> & x ) const
    {
        vector<float> ret;

        //if a single value <x,y> in the table, always return the y 
        if(_value_pairs.size( ) == 1)
        {
            ret.resize( x.size( ), _value_pairs.at( 0 ).second );
            return ret;
        }

        //if the table has no entries, it just returns the input x
        if(_value_pairs.size( ) < 1)
        {
            copy( x.begin( ), x.end( ), back_inserter( ret ) );
            return ret;
        }

        //returns the interpolated y value between two indices in the table 
        //const vector< pair<float, float>> &mp = value_pairs;
        function< float( float, int, int )> get_pair_interpolate( [this]( float v, int l1, int l2 )
            {
                const auto& epsilon = 0.00001f * (_value_pairs[l1].first + _value_pairs[l2].first);
                const auto& w0 = fabs( 1 / (v - _value_pairs[l1].first + epsilon) );
                const auto& w1 = fabs( 1 / (v - _value_pairs[l2].first + epsilon) );
                return ((_value_pairs[l1].second * w0 + _value_pairs[l2].second * w1) / (w0 + w1));
            } );

        function< pair<int, int>( float, int, int )> split( [this]( float number, int k1, int k2 )
            {
                if(k2 - k1 <= 1) return std::make_pair( k1, k2 );

                const int pivot = k1 + (int)(0.5 * (k2 - k1));
                if(number > _value_pairs[pivot].first)
                {
                    k1 = pivot;
                }
                else
                {
                    k2 = pivot;
                }
                return std::make_pair( k1, k2 );
            } );

        for(const auto& v : x)
        {
            if(v <= _value_pairs[0].first) ret.push_back( _value_pairs[0].second );
            else if(v >= _value_pairs[_value_pairs.size( ) - 1].first)  ret.push_back( _value_pairs[_value_pairs.size( ) - 1].second );
            else
            {
                int k1, k2;
                tie( k1, k2 ) = split( v, 0, (int)x.size( ) - 1 );

                while(k2 > k1 + 1)
                {
                    tie( k1, k2 ) = split( v, k1, k2 );
                }
                ret.push_back( get_pair_interpolate( v, k1, k2 ) );
            }
        }

        return ret;
    }


    private:

    vector< pair<float, float>> _value_pairs;

};


#endif 