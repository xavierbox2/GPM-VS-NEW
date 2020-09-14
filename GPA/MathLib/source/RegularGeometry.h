#ifndef GEOMETRY_H_
#define  GEOMETRY_H_ 1

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
This type represents a structured and regular in XY grid. the grid consists in a number of surfaces + the horizontal definition
of sizes. This grid spacing can be different in x and Y but it is constant along each direction. The spacing in z can be heterogeneous

This is the kind of grid that matches gpm data structure

The VS geometry structure is different, since it is structured but irregular in general.  Still, we will use this structure
and manage potential node displacements though the deck writter.
*/
class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
StructuredGrid: public StructuredBase, public IGridGeometry
{
    public:

        virtual  ~StructuredGrid() {  }

        StructuredGrid* operator->() { return this; }

        StructuredGrid() = default;

        StructuredGrid(const int ncols, const int nrows, const int nlayers, fVector3 extent, const CoordinateMapping3D &map);

        StructuredGrid& operator = (StructuredGrid &g);

        //<x1,y1,z1, <x2,y2,z2>, ...for surface
        vector<fVector3> get_local_coordinates_vector(int surface_index) const;

        //x1,y1,z1, x2,y2,z2, ...
        std::vector<float> get_local_coordinates() const;
        
        //x1,y1,z1, x2,y2,z2, ...for surface
        std::vector<float> get_local_coordinates(int surface_index) const;

        std::vector<float>::iterator surface_height_begin(int k) { return _zvalues[k].begin(); }

        std::vector<float>::iterator surface_height_end(int k) { return _zvalues[k].end(); }

        tuple< vector<float>::iterator, vector<float>::iterator> surface_height_begin_end(int k)
        {
            return make_tuple(surface_height_begin(k), surface_height_end(k));
        }


        void set_z_values(int surface_index, std::vector<float>& values)
        {
            add_surfaces_if_needed(surface_index);
            std::copy(values.begin(), values.end(), _zvalues[surface_index].begin());
        }

        void set_z_values(int surface_index,  float value)
        {
            add_surfaces_if_needed(surface_index);
            for( auto &v : _zvalues[surface_index]) v = value;
        }

        void set_z_values(int surface_index, float *values)
        {
            add_surfaces_if_needed(surface_index);
            std::vector<float>& z = _zvalues[surface_index];
            std::copy(values, values + z.size(), _zvalues[surface_index].begin());
        }









        void displace_all_nodes(std::vector<float> &displacement);

  


        void set_num_surfaces(int nz)
        {
            _node_count[2] = nz;
            add_surfaces_if_needed(_node_count[2] - 1);
        }

        fVector2 horizontal_spacing() const
        {
          return fVector2(_length[0] / (ncols() - 1), _length[1] / (nrows() - 1));
        }

        std::vector<float> nodal_to_elemental(int cols, int rows, int surfaces, const std::vector<float> &nodal_values);

        static std::vector<float>  elemental_to_nodal(int cols, int rows, int surfaces, const std::vector<float> &elemental_values);


        std::vector<float>& get_local_depths(int nk) { return _zvalues[nk]; }




        std::vector<float> get_local_depths(int nk) const
            {
                return _zvalues[nk];
            }

        std::pair<int,int> get_node_indices(int nk)
            {
            //first nodes_per_layer are for surface  = 0; , [nodes_per_layer, 2*nodes_per_layer ] -> surface = 1 ,....
            int n1 = nk * nodes_per_layer();
            return std::pair<int, int>(n1, n1 + nodes_per_layer());
            }

        StructuredSurface get_structured_surface(int index)
              {
                  return StructuredSurface(ncols(), nrows(), length(), reference(), _zvalues[index]);
              }

std::vector<int> get_height_overlaps(int surface1, int surface2, float tolerance) const;

std::tuple<int, int, int, int, int> get_geometry_description() const
  {
   return std::make_tuple(ncols(), nrows(), nsurfaces(), total_nodes(), total_elements());
  }

std::vector<int> get_element_indices_for_node(int node);

std::tuple<int, int, int, int> get_element_indices(int element);

std::tuple<int, int, int, int> get_node_indices_for_element(int element);

void brute_force_get_pinched_elements(float pinchout_tolerance, vector<int>  &element_pinched_count, map<int, int> &node_connections);

void get_side_node_indices(vector< vector<int> > &node_indices);

protected:

  vector<int> get_element_indices_above_node(int node);

  void add_surfaces_if_needed(int surface_index)
    {
     while (surface_index >= _zvalues.size())
     _zvalues.push_back(std::vector<float>(nodes_per_layer()));
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

        virtual ~StructuredDeformableGrid() {  }

        void add_displacements(const std::vector<float> &disp, int dir)
        {
        throw exception("Not implemented yet [StructuredDeformableGrid::add_displacements( const std::vector<float> &disp, int dir )] ");
        }

         std::vector<float> get_local_coordinates() const { return vector<float>(); }

         std::vector<float> get_local_coordinates(int surface_index) const { return vector<float>(); }

         void get_local_coordinates_vector(int surface_index, std::vector<fVector3> &ret) const { ; }

         std::vector<fVector3> get_local_coordinates_vector(int surface_index) const { return vector<fVector3>(); }

         std::vector<int> get_height_overlaps(int surface1, int surface2, float tolerance) const { return vector<int>(); }

         void brute_force_get_pinched_elements(float pinchout_tolerance, vector<int>  &element_pinched_count, map<int, int> &node_connections) { ; }

        void operator = (StructuredDeformableGrid &g) { ; }

    protected:

        //cummulated displacements
         std::vector<std::vector<float>> _xdisp;
         std::vector<std::vector<float>> _ydisp;
         std::vector<std::vector<float>> _zdisp;
};

#endif
