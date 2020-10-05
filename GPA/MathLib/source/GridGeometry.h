#ifndef GRID_GEOMETRY_H_
#define  GRID_GEOMETRY_H_ 1

#include "StructuredBase.h"

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    IGridGeometry
{
public:

    virtual ~IGridGeometry() {};

    virtual std::vector<float> get_local_coordinates() const = 0;

    virtual std::vector<float> get_local_coordinates(int surface_index) const = 0;

    virtual std::vector<fVector3> get_local_coordinates_vector(int surface_index) const = 0;

    virtual std::vector<float>& get_local_depths(int nk) = 0;

    virtual std::vector<float> get_local_depths(int nk) const = 0;

    virtual std::vector<float> get_local_depths(  ) const = 0;

    virtual std::vector<float> get_depths_from_top( ) const { return std::vector<float>();}


};

#endif
