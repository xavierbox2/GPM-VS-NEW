#include "CoordinateMapping.h"
#include "Vector3.h"
#include <functional>

std::ostream& operator<<(std::ostream &os, const CoordinateMapping3D &p)
{
    os << "<CoordinateMapping>" << std::endl;
    os << " <Origin> " << p.origin << " </Origin>" << std::endl;
    os << " <Axis1>  " << p.axis1 << " </Axis1>" << std::endl;
    os << " <Axis2>  " << p.axis2 << " </Axis2>" << std::endl;
    os << " <Axis3>  " << p.axis3 << " </Axis3>" << std::endl;
    os << "</CoordinateMapping>\n";
    return os;
}



CoordinateMapping3D::CoordinateMapping3D()
{
    axis1 = fvector3(1., 0, 0);
    axis2 = fvector3(0., 1., 0);
    axis3 = fvector3(0., 0, 1.);
    origin = fvector3(0., 0, 0.);
}

CoordinateMapping3D::CoordinateMapping3D(const fvector3 &a1, const fvector3 &a2, const fvector3 &a3, fvector3 o)
:axis1(a1), axis2(a2),axis3(a3),origin(o)
{;
}

CoordinateMapping3D::CoordinateMapping3D(float *a1, float *a2, float *a3)
{
    axis1 = fvector3(a1);
    axis2 = fvector3(a2);
    axis3 = fvector3(a3);
    origin = fvector3(0., 0, 0.);
}

CoordinateMapping3D& CoordinateMapping3D::operator=(const CoordinateMapping3D &c)
{
    if (this != &c)
    {
        axis1 = c.axis1;
        axis2 = c.axis2;
        axis3 = c.axis3;
        origin = c.origin;
    }
    return *this;
}

void CoordinateMapping3D::convert_to(std::vector<float> &source_xyz, std::vector<float> &target_xyz, const CoordinateMapping3D &c) const
{
    fvector3 rrprime = c.origin - origin;
    auto[a11, a12, a13, a21, a22, a23, a31, a32, a33] = get_coefficients(c);

    target_xyz.resize(source_xyz.size());
    int count = static_cast<int>(source_xyz.size()) / DIMS;
    for (int n = 0; n < count; n++)
    {
        int k = DIMS * n;
        target_xyz[k] = a11 * source_xyz[k] + a12 * source_xyz[k + 1] + a13 * source_xyz[k + 2] + rrprime[0];
        target_xyz[k + 1] = a21 * source_xyz[k] + a22 * source_xyz[k + 1] + a23 * source_xyz[k + 2] + rrprime[1];
        target_xyz[k + 2] = a31 * source_xyz[k] + a32 * source_xyz[k + 1] + a33 * source_xyz[k + 2] + rrprime[2];
    }
}

void CoordinateMapping3D::convert_to(const CoordinateMapping3D &c, std::vector<fVector3> &source_xyz) const
{
    fvector3 rrprime = c.origin - origin;
    auto [a11, a12, a13, a21, a22, a23, a31, a32, a33] = get_coefficients( c);

    std::vector<fVector3> target_xyz;
    target_xyz.resize(source_xyz.size());

    int count = static_cast<int>(source_xyz.size());
    for (int n = 0; n < count; n++)
    {
        int k = n;
        target_xyz[k][0] = a11 * source_xyz[k][0] + a12 * source_xyz[k][1] + a13 * source_xyz[k][2] + rrprime[0];
        target_xyz[k][1] = a21 * source_xyz[k][0] + a22 * source_xyz[k][1] + a23 * source_xyz[k][2] + rrprime[1];
        target_xyz[k][2] = a31 * source_xyz[k][0] + a32 * source_xyz[k][1] + a33 * source_xyz[k][2] + rrprime[2];
    }

    std::copy(target_xyz.begin(), target_xyz.end(), source_xyz.begin());
}

//Fix this so we dont do a copy
void CoordinateMapping3D::convert_to(const CoordinateMapping3D &c, std::vector<float> &source_xyz) const
{
    //inefficient copy
    std::vector<float> target_xyz;
    convert_to(source_xyz, target_xyz, c);

    std::copy(target_xyz.begin(), target_xyz.end(), source_xyz.begin());
}

void CoordinateMapping3D::convert_to(const float *x_vals, const float *y_vals, float **surfaces, int nxy_count, int surface_count, std::vector<float> &xyz, const CoordinateMapping3D &c, bool start_from_top) const
{
    fvector3 rrprime = c.origin - origin;
    auto [a11, a12, a13, a21, a22, a23, a31, a32, a33] = get_coefficients( c );

    xyz.resize(3 * nxy_count * surface_count);
    int index = 0;
    for (int ns = surface_count - 1; ns >= 0; ns--)
    {
        float *z_vals = surfaces[ns];

        for (int node = 0; node < nxy_count; node++)
        {
            float x = x_vals[node], y = y_vals[node], z = z_vals[node];

            float xprime = a11 * x + a12 * y + a13 * z + rrprime[0];
            float yprime = a21 * x + a22 * y + a23 * z + rrprime[1];
            float zprime = a31 * x + a32 * y + a33 * z + rrprime[2];

            xyz[index] = xprime;
            xyz[index + 1] = yprime;
            xyz[index + 2] = zprime;
            index += 3;
        }
    }
}

fvector3 CoordinateMapping3D::convert_to(const fvector3 &v1, const CoordinateMapping3D &c) const 
{
    fvector3 rrprime = c.origin - origin;
    auto [a11, a12, a13, a21, a22, a23, a31, a32, a33] = get_coefficients( c );

    float xprime = a11 * v1[0] + a12 * v1[1] + a13 * v1[2] + rrprime[0];
    float yprime = a21 * v1[0] + a22 * v1[1] + a23 * v1[2] + rrprime[1];
    float zprime = a31 * v1[0] + a32 * v1[1] + a33 * v1[2] + rrprime[2];

    return fvector3(xprime, yprime, zprime);
}


std::tuple<float, float, float, float, float, float, float, float, float> CoordinateMapping3D::get_coefficients(const CoordinateMapping3D &c) const noexcept
{
    fvector3 a1prime = c.axis1;
    fvector3 a2prime = c.axis2;
    fvector3 a3prime = c.axis3;

    float a11 = axis1 * a1prime, a12 = axis2 * a1prime, a13 = axis3 * a1prime;
    float a21 = axis1 * a2prime, a22 = axis2 * a2prime, a23 = axis3 * a2prime;
    float a31 = axis1 * a3prime, a32 = axis2 * a3prime, a33 = axis3 * a3prime;

    return std::make_tuple(a11,a12,a13,a21,a22,a23,a31,a32,a33);

}

 