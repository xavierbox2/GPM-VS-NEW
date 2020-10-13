/** Copyright (c) Schlumberger 2020. All rights reserved.
 *  GPM simulator plugin API
 *
 * Disclaimer
 * Use of this product is governed by the License Agreement. Schlumberger
 * makes no warranties, express, implied, or statutory, with respect to the
 * product described herein and disclaims without limitation any warranties of
 * merchantability or fitness for a particular purpose. Schlumberger reserves the
 * right to revise the information in this manual at any time without notice.
*/

/** Warranty Disclaimer:
  *
  * The APIs exposed in this package are not subject to restrictions regarding breaking changes.
  * Although it is Schlumberger Information Solutions' ambition that these APIs will continue to work as they originally worked
  * and any existing applications that use the APIs will continue to work without changes, it is expected that the process of
  * commercialization and future architectural changes will necessitate incompatible changes.
*/

#ifndef gpm_plugin_api_lin_multi_span_h
#define gpm_plugin_api_lin_multi_span_h

#include "gpm_plugin_lin_span.h"
#include <algorithm>
#include <numeric>
#include <cstddef>
#include <cassert>
#include <array>

namespace Slb { namespace Exploration { namespace Gpm { namespace Api {
#ifdef WIN32
#define GPM_DO_INLINE __forceinline
#else
#define GPM_DO_INLINE inline
#endif
template <typename eT, int N>
struct lin_multi_span {
    std::size_t dims[N];
    std::ptrdiff_t stride[N];
    lin_span<eT> holder;
    GPM_DO_INLINE lin_multi_span(): holder()
    {
        std::fill(std::begin(dims), std::end(dims), 0);
        std::fill(std::begin(stride), std::end(stride), 0);
    };
    GPM_DO_INLINE lin_multi_span(eT* ptr, const std::size_t (&loc_dims)[N], const std::ptrdiff_t (&loc_strides)[N])
    {
        assert(ptr != nullptr);
        //		assert(std::any_of(std::begin(loc_dims), std::end(loc_dims), 0)==false);
        std::copy(std::begin(loc_dims), std::end(loc_dims), std::begin(dims));
        std::copy(std::begin(loc_strides), std::end(loc_strides), std::begin(stride));
        auto nel = std::accumulate(std::begin(dims), std::end(dims), std::size_t(1), std::multiplies<std::size_t>());
        holder = lin_span<eT>(ptr, nel);
    };
    GPM_DO_INLINE lin_multi_span(eT* ptr, const std::array<std::size_t, N>& loc_dims,
                                 const std::array<std::ptrdiff_t, N>& loc_strides)
    {
        assert(ptr != nullptr);
        //		assert(std::any_of(std::begin(loc_dims), std::end(loc_dims), 0)==true);
        std::copy(std::begin(loc_dims), std::end(loc_dims), std::begin(dims));
        std::copy(std::begin(loc_strides), std::end(loc_strides), std::begin(stride));
        auto nel = std::accumulate(std::begin(dims), std::end(dims), std::size_t(1), std::multiplies<std::size_t>());
        holder = lin_span<eT>(ptr, nel);
    };
    // indexes contains {i,j,k}
    GPM_DO_INLINE eT& operator[](const std::ptrdiff_t (&indexes)[N])
    {
        auto adress = std::inner_product(std::begin(stride), std::end(stride), std::begin(indexes), std::ptrdiff_t(0));
        return holder[adress];
    }

    GPM_DO_INLINE const eT& operator[](const std::ptrdiff_t (&indexes)[N]) const
    {
        auto adress = std::inner_product(std::begin(stride), std::end(stride), std::begin(indexes), std::ptrdiff_t(0));
        return holder[adress];
    }

    GPM_DO_INLINE eT& operator[](const std::array<std::ptrdiff_t, N>& indexes)
    {
        auto adress = std::inner_product(std::begin(stride), std::end(stride), std::begin(indexes), std::ptrdiff_t(0));
        return holder[adress];
    }

    GPM_DO_INLINE const eT& operator[](const std::array<std::ptrdiff_t, N>& indexes) const
    {
        auto adress = std::inner_product(std::begin(stride), std::end(stride), std::begin(indexes), std::ptrdiff_t(0));
        return holder[adress];
    }

    GPM_DO_INLINE eT* begin() const
    {
        return holder.mem;
    }

    GPM_DO_INLINE eT* end() const
    {
        return holder.end();
    }

    GPM_DO_INLINE std::size_t size() const
    {
        return holder.nel;
    }
};
}}}}

#endif
