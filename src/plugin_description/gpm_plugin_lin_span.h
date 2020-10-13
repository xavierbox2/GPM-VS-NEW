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

#ifndef gpm_plugin_api_lin_span_h
#define gpm_plugin_api_lin_span_h

#include <cassert>
#include <cstddef>


namespace Slb { namespace Exploration { namespace Gpm { namespace Api {

/**
 @brief mimic std::span before that comes as part of the c++ standard
 */
#ifdef WIN32
#define GPM_DO_INLINE __forceinline
#else
#define GPM_DO_INLINE inline
#endif
template <typename eT>
struct lin_span {
    eT* mem;
    std::size_t nel;
    GPM_DO_INLINE lin_span()
    {
        mem = nullptr;
        nel = 0;
    };
    GPM_DO_INLINE lin_span(eT* ptr, std::size_t num)
    {
        assert(ptr != nullptr && num > 0);
        mem = ptr;
        nel = num;
    };
    GPM_DO_INLINE eT& operator[](const ptrdiff_t i)
    {
        assert(i >= 0 && i < nel);
        return mem[i];
    }

    GPM_DO_INLINE const eT& operator[](const ptrdiff_t i) const
    {
        assert(i >= 0 && i < nel);
        return mem[i];
    }

    GPM_DO_INLINE eT* begin() const
    {
        return mem;
    }

    GPM_DO_INLINE eT* end() const
    {
        return mem + nel;
    }

    GPM_DO_INLINE std::size_t size() const
    {
        return nel;
    }
};
}}}}

#endif
