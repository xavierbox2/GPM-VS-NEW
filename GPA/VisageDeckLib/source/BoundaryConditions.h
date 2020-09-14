#ifndef _BOUNDARY_CONDITIONS_H_
#define _BOUNDARY_CONDITIONS_H_ 1

#include <vector>
#include  <algorithm>
#include <iostream>

#ifdef MATHLIB_EXPORTS
#define MATHLIB_API __declspec(dllexport)
#else
#define MATHLIB_API __declspec(dllimport)
#endif

enum class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    BoundaryConditionType {
    STRAIN, DISPLACEMETSURFACE, FIXEDSURFACE
};
//DISPLACEORSTAY is really a displacement but if the displacement isnt provided, it is assumed to be zero just as in a FIXEDSURFACE condition.
//We use DISPLACEORSTAY for the basement surface

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    IBoundaryCondition
{
public:

    IBoundaryCondition(BoundaryConditionType t, int d) : _type(t), _dir(d) { ; }

    IBoundaryCondition(BoundaryConditionType t) : _type(t) { ; }

    virtual ~IBoundaryCondition() {}

    int dir() const { return _dir; }

    int& dir() { return _dir; }

    BoundaryConditionType type() const { return _type; }

    BoundaryConditionType& type() { return _type; }

    virtual void copy(IBoundaryCondition* b)
    {
        _dir = b->_dir;
        _type = b->_type;
    }

protected:

    BoundaryConditionType _type;

    int _dir;
};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    FixesBoundaryCondition final : public IBoundaryCondition
{
public:

    FixesBoundaryCondition(int dir = 0) :IBoundaryCondition(BoundaryConditionType::FIXEDSURFACE, dir)
    {
    }
    virtual ~FixesBoundaryCondition() override {}

    virtual void copy(IBoundaryCondition* b)
    {
        IBoundaryCondition::copy(b);
    }
};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    StrainBoundaryCondition final : public IBoundaryCondition
{
public:

    StrainBoundaryCondition(int dir = 0, float strn = 0.0) :IBoundaryCondition(BoundaryConditionType::STRAIN, dir)
    {
        _strain = strn;
    }

    virtual void copy(IBoundaryCondition* b)
    {
        IBoundaryCondition::copy(b);
        _strain = static_cast<StrainBoundaryCondition*>(b)->_strain;
    }

    float strain() const { return _strain; }

    float& strain() { return _strain; }

    float _strain;
};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
DisplacementSurfaceBoundaryCondition: public IBoundaryCondition
{
public:

    DisplacementSurfaceBoundaryCondition(int dir, const std::vector<float> &node_displacement)
    :IBoundaryCondition(BoundaryConditionType::DISPLACEMETSURFACE, dir)
    {
        set_node_displacement(node_displacement);
    }

    DisplacementSurfaceBoundaryCondition(int dir)
    :IBoundaryCondition(BoundaryConditionType::DISPLACEMETSURFACE, dir)
    {
        //*///_nodal_displacement.resize(0);
    }

    ~DisplacementSurfaceBoundaryCondition() { std::cout << "Deleting DisplacementSurfaceBoundaryCondition boundary condition" << std::endl; }

    virtual void copy(IBoundaryCondition* b)
    {
        IBoundaryCondition::copy(b);
        set_node_displacement(static_cast<DisplacementSurfaceBoundaryCondition*>(b)->get_node_displacement());
    }

    void clear_displacement()
    {
    _nodal_displacement.resize(0);
    }

    void set_node_displacement(const std::vector<float> &disp)
    {
        _nodal_displacement.resize(disp.size());
        std::copy(disp.cbegin(), disp.cend(), _nodal_displacement.begin());
    }

    void set_node_displacement(float *disp, int num)
    {
        _nodal_displacement.resize(num);
        std::copy(disp ,  disp + num, _nodal_displacement.begin());
    }

    std::vector<float>&  get_node_displacement()
    {
        return _nodal_displacement;
    }

    std::vector<float> _nodal_displacement;

    virtual void print()
    {
        //IBoundaryCondition::print();
        std::cout << "Type: DisplacementSurfaceBoundaryCondition (" << static_cast<int>(_type) << ") Dir: " << _dir << std::endl;
        std::cout << "Values: " << _nodal_displacement.size() << std::endl;
    }
};

#endif
