#include "StructuredSurface.h"

std::vector<float> StructuredSurface::get_local_coordinates() const 
{
    fVector2 spacing = horizontal_spacing();
    std::vector<float> ret(ncols()*nrows() * DIMS);
    const std::vector<float> &z = _zvalues;

    int counter = 0;
    for (int nj = 0; nj < nrows(); nj++)
    {
        float x2 = spacing[1] * nj;

        for (int ni = 0; ni < ncols(); ni++)
        {
            float x1 = spacing[0] * ni;
            float zval = z[counter];
            ret[DIMS*counter] = x1;
            ret[DIMS*counter + 1] = x2;
            ret[DIMS*counter + 2] = zval;
            counter += 1;
        }
    }

    return ret;
}
