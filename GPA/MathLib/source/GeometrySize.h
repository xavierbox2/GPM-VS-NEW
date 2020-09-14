#ifndef _GEOMETRY_SIZE_H_
#define _GEOMETRY_SIZE_H_ 1

#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

#include <vector>

struct
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    Structured_Geometry_Size
{
public:

    Structured_Geometry_Size* operator->() { return this; }

    Structured_Geometry_Size() :num_nodes_k(0), num_nodes_j(0), num_nodes_i(0) { ; }

    Structured_Geometry_Size(int nodes_i, int nodes_j, int nodes_k) :
        num_nodes_k(nodes_k), num_nodes_j(nodes_j), num_nodes_i(nodes_i) {
        ;
    }

    void operator=(const Structured_Geometry_Size &g)
    {
        num_nodes_k = g.num_nodes_k;
        num_nodes_j = g.num_nodes_j;
        num_nodes_i = g.num_nodes_i;
    }

    int num_nodes() const { return num_nodes_k * num_nodes_j * num_nodes_i; }

    int num_cells() const { return (num_nodes_k - 1)* (num_nodes_j - 1) * (num_nodes_i - 1); }

    int num_nodes_k, num_nodes_j, num_nodes_i;
};

#endif
