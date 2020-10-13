#ifndef _UTILS_H_
#define _UTILS_H_ 1

#include <vector>
#include <algorithm>
#include <numeric>
#include <iterator>
#include <string>
#include <memory>
#include <chrono>
#include <iostream>

using std::string;

class ChronoPoint
{
public:

    ChronoPoint( string s = " " )
    {
        name = s;
        start = std::chrono::system_clock::now( );
    }

    ~ChronoPoint( )
    {
        std::chrono::system_clock::time_point end = std::chrono::system_clock::now( );
        std::cout << "********Elapsed time*******" << name
            << " (ms): " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count( )
            << " (us): " << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count( )
            << std::endl;
    }

    long seconds( ) const
    {
        std::chrono::system_clock::time_point end = std::chrono::system_clock::now( );
        return std::chrono::duration_cast<std::chrono::seconds>(end - start).count( );
    }

     string name; 
    std::chrono::system_clock::time_point start;
};



template<typename Source, typename Target, typename Condition, typename Value>
void push_back_selected( const Source& source, Target& target, Condition condition, Value value )
{
    for(auto it = source.cbegin( ); it != source.cend( ); it++)
    {
        if(condition( *it ) == true)
            target.push_back( value( *it ) );
    }
}


template<typename Source, typename Target, typename Condition, typename Transform>
void copy_if_transformed( const Source& source, Target& target, Condition c, Transform t)
{
    for(auto it = source.begin( ); it != source.end( ); it++)
    {
        if( c(*it) == true)
        target.push_back( t( *it ) );
    }
}

template<typename Source, typename Target, typename Func>
void push_back_transformed( const Source& source, Target& target, Func f )
{
    for(auto it = source.begin( ); it != source.end( ); it++)
    {
        target.push_back( f( *it ) );
    }
}


template< class It1, class Function >
void for_it( It1 it1, It1 it2, Function f )
{
    for(; it1 != it2; ++it1)f( *it1 );
}

template< class Container, class Function >
void forall( Container& c, Function f ) { for_it( c.begin( ), c.end( ), f ); }

template< class Container, class Function >
void for_range( Container& c, int n, int n2, Function f ) { for_it( c.begin( ) + n, c.begin( ) + n2, f ); }

bool finder( std::string s, std::initializer_list<std::string> l );


bool exact_match( std::string s, const std::vector<std::string> &l );

template<typename T>
std::shared_ptr<T[]> make_shared_array( int size ) 
{ return shared_ptr<T[]>( new T[size], []( T* p ) { delete[] p; } ); }


#endif

 
