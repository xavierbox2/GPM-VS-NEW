#include "StructuredGrid.h"
#include "Vector3.h"
#include "Range.h"


vector<float>& StructuredGrid::get_heights( int surface_index )
{
    return _zvalues[surface_index];
}

std::vector<float> StructuredGrid::get_local_coordinates( ) const
{
    std::vector<float> xyz;
    for(int nk : IntRange( 0, nsurfaces( ) ))
    {
        std::vector<float> layer = get_local_coordinates( nk );
        std::copy( layer.begin( ), layer.end( ), back_inserter( xyz ) );
    }

    return xyz;
}

//returns a vector [ (x11,x12,x13), (x21,x22,x23), (x31,x32,x33)...]  expressed in the local coordinates of the grid
//it is always assumed that the nodes are arranged so cols variest the fastest, then rows and then layers.
std::vector<float> StructuredGrid::get_local_coordinates( int surface_index ) const
{
    std::vector<float> ret( DIMS * ncols( ) * nrows( ) );
    fVector2 spacing = horizontal_spacing( );

    const std::vector<float>& z = _zvalues[surface_index];
    int counter = 0;

    for(int nj = 0; nj < nrows( ); nj++)
    {
        float x2 = spacing[1] * nj;

        for(int ni = 0; ni < ncols( ); ni++)
        {
            float x1 = spacing[0] * ni;
            float zval = z[counter];

            ret[DIMS * counter] = x1;
            ret[DIMS * counter + 1] = x2;
            ret[DIMS * counter + 2] = zval;
            counter += 1;
        }
    }

    return ret;
}

StructuredGrid::StructuredGrid( const int ncols, const int nrows, const int nlayers, fVector2 extent, const CoordinateMapping3D& map )
    :StructuredBase( ncols, nrows, nlayers, extent, map )
{
    while(nlayers > _zvalues.size( ))
        _zvalues.push_back( std::vector<float>( nodes_per_layer( ) ) );
}

std::vector<fVector3> StructuredGrid::get_local_coordinates_vector( int surface_index ) const
{
    std::vector<fVector3> ret( nodes_per_layer( ) );
    fVector2 spacing = horizontal_spacing( );

    const std::vector<float>& z = _zvalues[surface_index];
    int counter = 0;

    for(int nj = 0; nj < nrows( ); nj++)
    {
        float x2 = spacing[1] * nj;

        for(int ni = 0; ni < ncols( ); ni++)
        {
            ret[counter] = fvector3( spacing[0] * ni, x2, z[counter] );
            counter += 1;
        }
    }

    return ret;
}

void StructuredGrid::displace_all_nodes( const std::vector<float>& displacement )
{
    ////xgt: note that i MUST be defined outside the capture block and captured by reference.
    int i = 0;
    for(int n : IntRange( 0, nsurfaces( ) ))
    {
        std::for_each( begin( _zvalues[n] ), end( _zvalues[n] ), [&displacement, &i]( float& h ){h += displacement[i++]; } );
    }
}

StructuredGrid& StructuredGrid::operator = ( StructuredGrid& g )
{
    if(&g != this)
    {
        StructuredBase::operator=( g );

        _zvalues.resize( g._zvalues.size( ) );

        for(int i = 0; i < g._zvalues.size( ); i++)
        {
            _zvalues[i].resize( g._zvalues[i].size( ) );
            std::copy( g._zvalues[i].begin( ), g._zvalues[i].end( ), _zvalues[i].begin( ) );
        }
    }

    return *this;
}

std::vector<int>  StructuredGrid::get_height_overlaps( int surface1, int surface2, float tolerance ) const
{
    std::vector<int> nodes;
    std::vector<float>& s1 = get_local_depths( surface1 );
    std::vector<float>& s2 = get_local_depths( surface2 );

    for(int i : IntRange( 0, (int)s1.size( ) ))
    {
        if(fabs( s1[i] - s2[i] ) < tolerance)
            nodes.push_back( i );
    }

    return nodes;
}



void StructuredGrid::brute_force_get_pinched_elements( float pinchout_tolerance, vector<int>& element_pinched_count, map<int, int>& node_connections )
{

    element_pinched_count.resize( num_cells( ), 0 ); 
    node_connections.clear( );

    for(int k1 = 0; k1 < nsurfaces( ) - 1; k1++)
    {
        int k2 = k1 + 1;
        vector<float>& heights_below = get_local_depths( k1 );
        vector<float>& heights_above = get_local_depths( k2 );

        int nodes_per_surface = (int)heights_above.size( );

        for(auto n : IntRange( 0, nodes_per_surface ))
        {
            //float difference = fabs( heights_above[n] - heights_below[n] );
            float difference = heights_above[n] - heights_below[n];
            if(difference < pinchout_tolerance)
            {
                int node1 = nodes_per_surface * k1 + n;
                int node2 = node1 + nodes_per_surface;
                std::vector<int> element_indices = get_element_indices_above_node( node1 );
                for(auto ele : element_indices)
                {
                    element_pinched_count[ele] += 1;
                }
                node_connections[node1] = node2;
            }
        }
    }
}



//void StructuredGrid::brute_force_get_pinched_elements( float pinchout_tolerance, vector<int>& element_pinched_count, map<int, int>& node_connections )
//{
//    int  n_surfaces = nsurfaces( ), nodes_per_surface = nodes_per_layer( );
//    node_connections.clear( );
//    element_pinched_count.resize( num_cells( ), 0 );
//
//    //surface below
//    for(int k1 = 0; k1 < n_surfaces - 1; k1++)
//    {
//        vector<float> heights_below = get_local_depths( k1 );
//
//        //surface above 
//        for(int k2 = k1 + 1; k2 < n_surfaces; k2++)
//        {
//            vector<float> heights_above = get_local_depths( k2 );
//
//
//            for(int n=0; n < nodes_per_surface; n++)
//            {
//                float difference = fabs( heights_above.at(n) - heights_below.at(n) );
//                if(difference < pinchout_tolerance)
//                {
//                    int node1 = nodes_per_surface * k1 + n;
//                    int node2 = nodes_per_surface * k2 + n;
//                    std::vector<int> element_indices = get_element_indices_above_node( node1 );
//                    for(auto ele : element_indices)
//                    {
//                        element_pinched_count[ele] += 1;
//                    }
//
//                    node_connections[node1] = node2;
//                }
//            }
//
//        }
//        
//
//    }
//}
