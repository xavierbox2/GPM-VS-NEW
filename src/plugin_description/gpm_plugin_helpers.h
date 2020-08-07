// -- Schlumberger Private --
#ifndef gpm_plugin_api_plugin_helpers_h
#define gpm_plugin_api_plugin_helpers_h

#include "gpm_plugin_lin_multi_span.h"
#include "gpm_plugin_description.h"
#include <vector>
#include <map>
#include <cstddef>
#include <cassert>

// These really should be GSL's multi_span
// But we need to wait for gcc 5.1 before we can start using that
// Meanwhile make a couple of simple ones
namespace Slb {
    namespace Exploration {
        namespace Gpm {
            namespace Api {
                template <class T>
                class array_2d_indexer : public lin_multi_span<T, 2> {
                public:
                    typedef T value_type;
                    typedef std::size_t size_type;
                    typedef lin_multi_span<T, 2> base_type;
                    array_2d_indexer( T* const arr, const gpm_plugin_api_2d_memory_layout& layout ) :base_type( arr, { layout.num_rows, layout.num_cols }, { layout.row_stride, layout.col_stride } )
                    {
                    }

                    T const& operator()( ptrdiff_t i, ptrdiff_t j ) const
                    {
                        return this->operator[]( { i,j } );
                    }

                    T& operator()( ptrdiff_t i, ptrdiff_t j )
                    {
                        return this->operator[]( { i,j } );
                    }

                    T const& get( ptrdiff_t i, ptrdiff_t j ) const
                    {
                        return this->operator[]( { i,j } );
                    }

                    T& get( ptrdiff_t i, ptrdiff_t j )
                    {
                        return this->operator[]( { i,j } );
                    }
                    std::size_t num_rows( ) const
                    {
                        return this->dims[0];
                    }
                    std::size_t num_cols( ) const
                    {
                        return this->dims[1];
                    }
                };

                template <class T>
                class array_3d_indexer :public lin_multi_span<T, 3> {
                public:
                    typedef T value_type;
                    typedef std::size_t size_type;
                    typedef lin_multi_span<T, 3> base_type;

                    array_3d_indexer( T* const arr, const gpm_plugin_api_3d_memory_layout& layout ) : base_type(
                        arr, {
                            layout.num_rows, layout.num_cols, layout.num_samples
                        }, { layout.row_stride, layout.col_stride, layout.sample_stride } )
                    {
                    }

                    T const& operator()( std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k ) const
                    {
                        return this->operator[]( { i,j,k } );
                    }

                    T& operator()( std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k )
                    {
                        return this->operator[]( { i,j,k } );
                    }

                    T const& get( std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k ) const
                    {
                        return this->operator[]( { i,j,k } );
                    }

                    T& get( std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k )
                    {
                        return this->operator[]( { i,j,k } );
                    }
                };

                // For sediment descriptions
                struct sediment_descr_type {
                    std::string name;
                    std::string id;
                    int index_in_array{};
                };

                inline std::vector<std::string> make_attributes( const gpm_plugin_api_string_layout* attr_names, size_t num_attributes )
                {
                    std::vector<std::string> res;
                    res.reserve( num_attributes );
                    for(int i = 0; i < num_attributes; ++i) {
                        res.emplace_back( attr_names[i].str, attr_names[i].str_length );
                    }
                    return res;
                }

                inline std::map<std::string, std::vector<Api::array_2d_indexer<float>>>
                    make_array_holders( const gpm_plugin_api_process_attribute_parms& attrs )
                {
                    std::map<std::string, std::vector<Api::array_2d_indexer<float>>> res;
                    auto const_surf = attrs.surface_layout;
                    const_surf.col_stride = 0;
                    const_surf.row_stride = 0;
                    for(int i = 0; i < attrs.num_attributes; ++i) {
                        std::string name( attrs.attr_names[i].str, attrs.attr_names[i].str_length );
                        std::vector<Api::array_2d_indexer<float>> holders;
                        for(int j = 0; j < attrs.num_attr_array[i]; ++j) {
                            auto tmp = Slb::Exploration::Gpm::Api::array_2d_indexer<float>(
                                attrs.attributes[i][j], (attrs.is_constant[i][j] ? const_surf : attrs.surface_layout) );
                            holders.push_back( tmp );
                        }
                        res[name] = holders;
                    }
                    return res;
                }

                inline std::vector<sediment_descr_type>
                    make_sediment_descriptions( gpm_plugin_api_sediment_definition* seds, int num_seds )
                {
                    std::vector<sediment_descr_type> res;
                    for(int i = 0; i < num_seds; ++i)
                    {
                        sediment_descr_type item;
                        item.id = std::string( seds[i].id, seds[i].id_length );
                        item.name = std::string( seds[i].name, seds[i].name_length );
                        item.index_in_array = seds[i].index_in_sed_array;
                        res.push_back( item );
                    }
                    return res;
                }
            }
        }
    }
}
#endif
