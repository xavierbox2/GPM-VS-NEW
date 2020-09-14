#ifndef COORDINATE_MAPPING_H_
#define COORDINATE_MAPPING_H_ 1

/*
Xavier Garcia, xavierbox@gmail.com
Adapted from the DEM library.
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <tuple>
#include "Vector3.h"

#ifdef MATHLIB_EXPORTS
#define MATHLIB_API __declspec(dllexport)
#else
#define MATHLIB_API __declspec(dllimport)
#endif

/*
The class defines a coordinate system by threee axes and an origin (referred to the glocal cartesian).
Then some methods are provided to transform coordinates xyz represented in this coordinate frame to an arbitrary one.

Example:

CoordinateMapping3D c1(); //cartesian 

CoordinateMapping3D c2( {0.0,1.0,0.0}, {1.0,0.0,0.0}, {0.0,0.0,1.0},); //rotated. 

vector<float> coords = {....} //x1,y1,z1, x2,y2,z2,..... in the cartesian 

vector<float> transformed_coords = c1.to( c2, coords ) ////x1*,y1*,z1*, x2*,y2*,z2*,..... in the rotated 

*/
class
#ifdef ISDLL
    MATHLIB_API
#endif
    CoordinateMapping3D
{
    friend std::ostream& operator<<(std::ostream &os, const CoordinateMapping3D &p);

    static const int DIMS = 3;

public:

    CoordinateMapping3D* operator->() { return this; }

    //default: cartesian coordinate syatem
    CoordinateMapping3D();

    //create a coordinate system with axis a1,a2,a3 refereed to the cartesian
    explicit CoordinateMapping3D(const fvector3 &a1, const fvector3 &a2, const fvector3 &a3, fvector3 o = fvector3(0., 0, 0.));

    //create a coordinate system with axis a1,a2,a3 refereed to the cartesian
    explicit CoordinateMapping3D(float *a1, float *a2, float *a3);

    //deep copy 
    CoordinateMapping3D(const CoordinateMapping3D &c) = default; 

    //move semantics not needed.

    CoordinateMapping3D& operator=(const CoordinateMapping3D &c);

    /*
    All the following methods convert one or more (x,y,z) collections assumed to be in this syste, to another system. 
    All the methods can throw
    */

    void convert_to(std::vector<float> &source_xyz, std::vector<float> &target_xyz, const CoordinateMapping3D &c) const;

    void convert_to(const CoordinateMapping3D &c, std::vector<fVector3> &source_xyz) const;

    //Fix this so we dont do a copy
    void convert_to(const CoordinateMapping3D &c, std::vector<float> &source_xyz) const;

    //xavier gt: tested, unit test [0003] in file unit_tests.cpp
    //coordinate_mapping3: pass the gpm surfaces abs x,y, coordinates to obtain the coordinate transforms to visage or any other coordinate system
    //1-Create coordinate system for visage, example:
    //Coordinate_mapping3 vs( v1,v2,v3 ) where v1,v2,v3 are the isage axes in relation to world
    //2-Fetch the data from gpm, it from model definition, assume x3 is vertical
    //pass the members of gpm_plugin_api_process_with_model_geometry_parms
    //the method returns the xyz in a vector for the nodes, already in the best order to write the element connections and the node files
    void convert_to(const float *x_vals, const float *y_vals, float **surfaces, int nxy_count, int surface_count, std::vector<float> &xyz, const CoordinateMapping3D &c, bool start_from_top = true) const;

    fvector3 convert_to(const fvector3 &v1, const CoordinateMapping3D &c) const;
  
    fVector3 axis1, axis2, axis3, origin;

private:

    inline float dot(float *a, float *b) const noexcept
    {
        return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
    }

    //void get_coefficients(float &a11, float &a12, float &a13, float &a21, float &a22, float &a23, float &a31, float &a32, float &a33, const CoordinateMapping3D &c) const noexcept;
    std::tuple<float,float,float, float, float, float, float, float, float> get_coefficients(const CoordinateMapping3D &c) const noexcept;

};


#endif
