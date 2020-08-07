#ifndef GPM_ATT_ITERATOR_H 
#define GPM_ATT_ITERATOR_H 1

#include <numeric>
#include <algorithm>
#include <iterator>
#include <vector>

#include "gpm_plugin_helpers.h"

using namespace std;

class gpm_attribute_mock_surface
{
public:

    gpm_attribute_mock_surface()
    {
        _r = 0;
        _c = 0;
        _val = nullptr;

    }

    gpm_attribute_mock_surface(int row, int col)
    {
        alloc(row, col);
    }

    void alloc(int row, int col)
    {
        _r = row;
        _c = col;
        _val = new float*[_r];
        for (int n = 0; n < _r; n++)
        {
            _val[n] = new float[_c];
        }
    }

    ~gpm_attribute_mock_surface()
    {
        for (int n = 0; n < _r; n++)
        {
            delete[] _val[n];
        }

        delete[] _val;
    }



    float& operator()(int row, int col)
    {
        return _val[row][col];
    }

    float operator()(int row, int col) const
    {
        return _val[row][col];
    }

    size_t num_cols() const { return _c; }

    size_t num_rows() const { return _r; }

    int _r, _c;
    float** _val;
};

class gpm_attribute_mock
{
public:

    ~gpm_attribute_mock()
    {
        if (_val)
            delete[] _val;
    }

    explicit gpm_attribute_mock(int s, int r, int c) : _s(s), _r(r), _c(c)
    {
        _val = new gpm_attribute_mock_surface[_s];
        for (int n = 0; n < _s; n++)
            _val[n].alloc(_r, _c);

        int counter = 0;
        for (int s = 0; s < size(); s++)
        {
            for (int r = 0; r < num_rows(); r++)
            {
                for (int c = 0; c < num_cols(); c++)
                {
                    _val[s](r, c) = 1.0f* counter++;// s * 10 + r * 1 + 0.1*c;
                }
            }
        }
    }

    void print()
    {
        for (int s = 0; s < size(); s++)
        {
            for (int r = 0; r < num_rows(); r++)
            {
                for (int c = 0; c < num_cols(); c++)
                {
                    //cout << _val[s](r, c) << "  ";// s * 10 + r * 1 + 0.1*c;
                }
            }
        }

//        cout << endl;
    }

    size_t num_cols() const { return _c; }

    size_t num_rows() const { return _r; }

    size_t num_surfaces() const { return _s; }

    size_t size() const { return num_surfaces(); }

    gpm_attribute_mock_surface& operator[] (int n) { return _val[n]; }

private:

    int _s, _r, _c;
    gpm_attribute_mock_surface* _val;

};


template< typename ATT_TYPE>
class attribute_iterator
{
public:
    //using gpm_attribute = vector<Slb::Exploration::Gpm::Api::array_2d_indexer<float>>;
    //using gpm_attribute = gpm_attribute_mock;

    using  iterator_category = std::forward_iterator_tag;
    using  value_type = float;
    using  const_reference = const float&;
    using  reference = float&;
    using  pointer = float*;
    using  difference_type = int;//ptrdiff_t;
    using  self_type = attribute_iterator;

    //typedef attribute_iterator<ATT_TYPE> att_iterator;
    //typedef attribute_iterator<ATT_TYPE const> const_att_iterator;

    explicit attribute_iterator( ATT_TYPE &att, int e = 0)
        :_att(&att), _element(e){;}

    attribute_iterator(const attribute_iterator &it) = default;

    attribute_iterator() = default;

    attribute_iterator& operator=(const attribute_iterator &it) = default;

    bool operator==(const attribute_iterator &it) const noexcept
    {
        assert(compatible(it));
        return (_element == it._element);
    }

    bool operator!=(const self_type &it) const
    {
        return !(*this == it);
    }

    self_type operator++(int)  noexcept
    {
        self_type tmp(*_att, _element);
        this->operator++();
        return tmp;
    }

    self_type& operator++()  noexcept
    {
        _element += 1;
        return *this;
    }

    const_reference operator* () const
    {
        auto[s, r, c] = indices(_element);
        return  const_cast<float&>(_att->operator[](s)(r, c));
    }

    reference operator* ()
    {
        auto[s, r, c] = indices(_element);
        return (_att->operator[](s)(r, c));
    }

    const_reference operator-> () const
    {
        auto[s, r, c] = indices(_element);
        return (_att->operator[](s)(r, c));
    }

    static attribute_iterator end( ATT_TYPE &att)
    {
        return attribute_iterator(att, att.num_surfaces()*att.num_cols()*att.num_rows());
    }

    static attribute_iterator begin( ATT_TYPE &att)
    {
        return attribute_iterator(att, 0);
    }

    bool compatible(self_type const & other) const
    {
        return _att == other._att;
    }

private:

    size_t num_cols() const { return _att->operator[](0).num_cols(); }

    size_t num_rows() const { return _att->operator[](0).num_rows(); }

    size_t num_surfaces() const { return _att->size(); }

    size_t size() const { return num_surfaces(); }

    int element() const
    {
        return _element;//_col + _row * num_cols() + _surface * (num_cols()*num_rows());
    }

    tuple<int, int, int> indices(int el) const
    {
        int surf = (int)(el / (num_cols() * num_rows()));
        int ncols = num_cols();
        int nrows = num_rows();
        int elexy = el - surf * (num_cols() * num_rows());

        int row = elexy / num_rows();

        int col = el - row * num_cols() - surf * (num_cols()*num_rows());

        return make_tuple(surf, row, col);
    }

    ATT_TYPE* _att = nullptr;

    int _element;
};


    

    

#endif 

