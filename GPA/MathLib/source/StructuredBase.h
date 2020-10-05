#ifndef _STRUCTURED_BASE_H_
#define _STRUCTURED_BASE_H_ 1

#include <iostream>
#include <fstream>
#include <sstream>
#include <cmath>
#include <iomanip>
#include <numeric>
#include <vector>
#include <map>
#include <string>
#include <tuple>
#include <algorithm>

#include "Vector3.h"
#include "Range.h"
#include "CoordinateMapping.h"

#ifdef MATHLIB_EXPORTS
#define MATHLIB_API __declspec(dllexport)
#else
#define MATHLIB_API __declspec(dllimport)
#endif




 
using namespace std;

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    StructuredBase
{
    friend std::ostream& operator<<( std::ostream& os, StructuredBase& p )
    {
        os << "<StructuredBase>" << std::endl;
        os << " <ncols>  " << p.ncols( ) << " </ncols>" << std::endl;
        os << " <nrows>  " << p.nrows( ) << " </nrows>" << std::endl;
        os << " <length>  " << p.length( ) << " </length>" << std::endl;
        os << p.reference( );
        os << "</StructuredBase>\n";
        return os;
    }

public:

    const int DIMS{ 3 };

    virtual ~StructuredBase( ) { ; }

    StructuredBase( ) = default;

    StructuredBase& operator=( const StructuredBase& s )
    {
        _reference = s._reference; _length = s._length; _node_count = s._node_count;
        return *this;
    }

    StructuredBase( const int ncols, const int nrows, const int nlayers, fVector2 extent, const CoordinateMapping3D& map )
        :_reference( map ), _length( extent ), _node_count( ncols, nrows, nlayers ) {}

    StructuredBase( const int ncols, const int nrows, fVector2 extent, const CoordinateMapping3D& map )
        : StructuredBase( ncols, nrows, 1, extent, map ) {}

    inline fVector2 length( ) const noexcept { return _length; }

    inline fVector2 lateral_extent( ) const noexcept { return _length; }

    fVector2 horizontal_spacing( ) const
    {
        return fVector2( _length[0] / (ncols( ) - 1), _length[1] / (nrows( ) - 1) );
    }

    inline int ncols( ) const noexcept { return _node_count[0]; }

    inline int nrows( ) const noexcept { return _node_count[1]; }

    inline int total_nodes( ) const noexcept { return _node_count.trace_mult( ); };

    inline int num_cells( ) const noexcept { return (_node_count - 1).trace_mult( ); };

    inline int total_elements( ) const noexcept { return num_cells( ); };

    inline iVector3 node_count( ) const noexcept { return  _node_count; }

    inline iVector3 element_count( ) const noexcept { return iVector3( std::max<int>( 0, _node_count[0] - 1 ), std::max<int>( 0, _node_count[1] - 1 ), std::max<int>( 0, _node_count[2] - 1 ) ); }

    inline int nsurfaces( ) const noexcept { return nlayers( ); }

    inline int nlayers( ) const noexcept { return _node_count[2]; }

    inline int nodes_per_layer( ) const noexcept { return ncols( ) * nrows( ); }

    CoordinateMapping3D reference( ) const noexcept { return  _reference; }

    CoordinateMapping3D& reference( ) { return  _reference; }

    void get_side_node_indices( std::vector< std::vector<int> >& node_indices )
    {
        auto aux = nsurfaces( ) * (ncols( ) > nrows( ) ? ncols( ) : nrows( ));

        node_indices.resize( 4 ); //left i, rght i, left j, right j
        for(int n = 0; n < node_indices.size( ); n++)
            node_indices[n].reserve( aux );

        int counter = 0;
        for(int nk = 0; nk < nsurfaces( ); nk++)
        {
            for(int nj = 0; nj < nrows( ); nj++)
            {
                for(int ni = 0; ni < ncols( ); ni++)
                {
                    if(ni == 0)
                    {
                        node_indices[0].push_back( counter );
                    }
                    else if(ni == ncols( ) - 1)
                    {
                        node_indices[1].push_back( counter );
                    }
                    else { ; }

                    if(nj == 0)
                    {
                        node_indices[2].push_back( counter );
                    }
                    else if(nj == nrows( ) - 1)
                    {
                        node_indices[3].push_back( counter );
                    }
                    else { ; }

                    counter += 1;
                }
            }
        }
    }

    static std::vector<float> nodal_to_elemental( int cols, int rows, int surfaces, const std::vector<float>& nodal_values )//, std::vector<float> &elemental_values)
    {
        std::vector<float> elemental_values;
        elemental_values.resize( (cols - 1) * (rows - 1) * (surfaces - 1) );

        int cell = 0;
        float one_eight = 1.0f / 8.0f;

        for(int k = 0; k < surfaces - 1; k++)
        {
            for(int j = 0; j < rows - 1; j++)
            {
                for(int i = 0; i < cols - 1; i++)
                {
                    //top face
                    int n1 = k * (cols * rows) + j * cols + i;
                    float value = nodal_values.at( n1 ) + nodal_values.at( n1 + 1 ) + nodal_values.at( n1 + cols ) + nodal_values.at( n1 + 1 + cols );

                    //the other face
                    n1 += (cols * rows);
                    value += nodal_values.at( n1 ) + nodal_values.at( n1 + 1 ) + nodal_values.at( n1 + cols ) + nodal_values.at( n1 + 1 + cols );

                    //aritmetic average
                    elemental_values[cell++] = value * one_eight;
                }
            }
        }

        return elemental_values;
    }

    static std::vector<float>  elemental_to_nodal( int cols, int rows, int surfaces, const std::vector<float>& elemental_values )
    {
        std::vector<float> nodal_values;
        nodal_values.resize( (cols) * (rows) * (surfaces) );
        std::vector<int> counter( (cols) * (rows) * (surfaces) );
        std::fill( nodal_values.begin( ), nodal_values.end( ), 0.0f );
        std::fill( counter.begin( ), counter.end( ), 0 );

        int cell = 0, aux = cols * rows;
        float one_eight = 1.0f / 8.0f;

        for(int k = 0; k < surfaces - 1; k++)
        {
            for(int j = 0; j < rows - 1; j++)
            {
                for(int i = 0; i < cols - 1; i++)
                {
                    //top face and bottom faces
                    int n1 = k * (cols * rows) + j * cols + i; //lower left node of cell
                    float value = elemental_values.at( cell );

                    int indices[] = { n1, n1 + 1, n1 + cols, n1 + 1 + cols };
                    for(int n = 0; n < 4; n++)
                    {
                        nodal_values[(indices[n])] += value;
                        counter[indices[n]] += 1;
                        indices[n] += aux;
                        nodal_values[(indices[n])] += value;
                        counter[indices[n]] += 1;
                    }

                    cell += 1;
                }
            }
        }//k

        for(int n = 0; n < nodal_values.size( ); n++) nodal_values[n] /= (counter[n]);

        return nodal_values;
    }

    std::vector<float>  elemental_to_nodal( const std::vector<float>& elemental_values ) const
    {
        return elemental_to_nodal( ncols( ), nrows( ), nsurfaces( ), elemental_values );
    }

    std::vector<float> nodal_to_elemental( const std::vector<float>& nodal_values ) const
    {
        return nodal_to_elemental( ncols( ), nrows( ), nsurfaces( ), nodal_values );
    }

    std::tuple<int, int, int, int, int> get_geometry_description( ) const
    {
        return std::make_tuple( ncols( ), nrows( ), nsurfaces( ), total_nodes( ), total_elements( ) );
    }

    std::pair<int, int> get_node_indices( int nk )
    {
    //first nodes_per_layer are for surface= 0; , [nodes_per_layer, 2*nodes_per_layer ] -> surface = 1 ,....
        int n1 = nk * nodes_per_layer( );
        return std::pair<int, int>( n1, n1 + nodes_per_layer( ) );
    }

    std::vector<int> get_element_indices_for_node( int node )
    {
        iVector3 nodes = node_count( );

        int n_k = int( node / (nodes[0] * nodes[1]) ),
            n_ij = node - n_k * (nodes[0] * nodes[1]),
            n_j = int( n_ij / (nodes[0]) ),
            n_i = n_ij - n_j * nodes[0];

        //std::cout<<"node "<<node<<" has indices  "<<n_i<<" "<<n_j<<" "<<n_k<<std::endl;

        std::vector<int> elements;
        iVector3 cells = element_count( );
        for(int ci = std::max<int>( n_i - 1, 0 ); ci <= std::min<int>( cells[0] - 1, n_i ); ci++)
        {
            for(int cj = std::max<int>( n_j - 1, 0 ); cj <= std::min<int>( cells[1] - 1, n_j ); cj++)
            {
                for(int ck = std::max<int>( n_k - 1, 0 ); ck <= std::min<int>( cells[2] - 1, n_k + 1 ); ck++)
                {
                    int cell = ci + cj * cells[0] + ck * (cells[0] * cells[1]);
                    elements.push_back( cell );
                }
            }
        }

        return elements;
    }

    std::tuple<int, int, int, int> get_element_indices( int element )
    {
        iVector3 cells = element_count( );

        int c_k = int( element / (cells[0] * cells[1]) ),
            c_ij = element - c_k * (cells[0] * cells[1]),
            cell_j = int( c_ij / (cells[0]) ),
            cell_i = c_ij - cell_j * cells[0];

        return std::make_tuple( c_k, c_ij, cell_i, cell_j );
    }

    std::tuple<int, int, int, int> get_node_indices_for_element( int element )
    {
        iVector3 nodes = node_count( );
        iVector3 cells = element_count( );
        int c_k, c_ij, cell_i, cell_j;//, cell_k;
        std::tie( c_k, c_ij, cell_i, cell_j ) = get_element_indices( element );

        int n1 = cell_j * nodes[0] + cell_i + c_k * (nodes[0] * nodes[1]),
            n2 = n1 + 1,
            n3 = n2 + nodes[0],
            n4 = n1 + nodes[0];

        return std::make_tuple( n1, n2, n3, n4 );
    }

    std::vector<int> get_element_indices_above_node( int node )
    {
        iVector3 nodes = node_count( );

        int n_k = int( node / (nodes[0] * nodes[1]) ),
            n_ij = node - n_k * (nodes[0] * nodes[1]),
            n_j = int( n_ij / (nodes[0]) ),
            n_i = n_ij - n_j * nodes[0];

        std::vector<int> elements;
        iVector3 cells = element_count( );
        for(int ci = std::max<int>( n_i - 1, 0 ); ci <= std::min<int>( cells[0] - 1, n_i ); ci++)
        {
            for(int cj = std::max<int>( n_j - 1, 0 ); cj <= std::min<int>( cells[1] - 1, n_j ); cj++)
            {
                {   int ck = n_k;//std::max<int>(n_k - 1, 0);
                int cell = ci + cj * cells[0] + ck * (cells[0] * cells[1]);
                elements.push_back( cell );
                }
            }
        }

        return elements;
    }

protected:

    CoordinateMapping3D  _reference;

    fVector2 _length;

    iVector3 _node_count;
};

#endif
