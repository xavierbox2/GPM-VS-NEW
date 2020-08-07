#ifndef gpm_plugin_api_lin_span_h
#define gpm_plugin_api_lin_span_h

#include <cassert>
#include <cstddef>
#pragma warning(push, 0)

namespace Slb {
    namespace Exploration {
        namespace Gpm {
            namespace Api {
#ifdef WIN32
#define GPM_DO_INLINE __forceinline
#else
#define GPM_DO_INLINE inline
#endif
                template <typename eT>
                struct lin_span {
                    eT* mem;
                    std::size_t nel;
                    GPM_DO_INLINE lin_span( )
                    {
                        mem = nullptr;
                        nel = 0;
                    };
                    GPM_DO_INLINE lin_span( eT* ptr, std::size_t num )
                    {
                        assert( ptr != nullptr && num > 0 );
                        mem = ptr;
                        nel = num;
                    };
                    GPM_DO_INLINE eT& operator[]( const ptrdiff_t i )
                    {
                        assert( i >= 0 && i < nel );
                        return mem[i];
                    }

                    GPM_DO_INLINE const eT& operator[]( const ptrdiff_t i ) const
                    {
                        assert( i >= 0 && i < nel );
                        return mem[i];
                    }

                    GPM_DO_INLINE eT* begin( ) const
                    {
                        return mem;
                    }

                    GPM_DO_INLINE eT* end( ) const
                    {
                        return mem + nel;
                    }

                    GPM_DO_INLINE std::size_t size( ) const
                    {
                        return nel;
                    }
                };
            }
        }
    }
}
#pragma warning(pop)

#endif
