
#ifndef gpm_plugin_api_plugin_helpers_h
#define gpm_plugin_api_plugin_helpers_h
#include "gpm_plugin_lin_multi_span.h"
#include "gpm_plugin_description.h"
#include <vector>
#include <map>
#include <cstddef>
#include <cassert>
#include <string>

/** @file
 *
 *@brief
 * This file contains helper classes to make a more c++ view of the GPM public API
 * The classes are views and convenience classes to wrap to C structs
 */

namespace Slb { namespace Exploration { namespace Gpm { namespace Api {
/**
 @brief View of linear array as a 2d array. Based on GSL's multi_span
 */
template <class T>
class array_2d_indexer : public lin_multi_span<T, 2> {
public:
    typedef T value_type;
    typedef std::size_t size_type;
    typedef lin_multi_span<T, 2> base_type;

    array_2d_indexer(): base_type()
    {
    }

    array_2d_indexer(T* const arr, const gpm_plugin_api_2d_memory_layout& layout) : base_type(
        arr, {layout.num_rows, layout.num_cols}, {layout.row_stride, layout.col_stride})
    {
    }

    T const& operator()(ptrdiff_t i, ptrdiff_t j) const
    {
        return this->operator[]({i, j});
    }

    T& operator()(ptrdiff_t i, ptrdiff_t j)
    {
        return this->operator[]({i, j});
    }

    T const& get(ptrdiff_t i, ptrdiff_t j) const
    {
        return this->operator[]({i, j});
    }

    T& get(ptrdiff_t i, ptrdiff_t j)
    {
        return this->operator[]({i, j});
    }

    std::size_t num_rows() const
    {
        return this->dims[0];
    }

    std::size_t num_cols() const
    {
        return this->dims[1];
    }
};

/**
 @brief View of linear array as a 3d array. Based on GSL's multi_span
 */
template <class T>
class array_3d_indexer : public lin_multi_span<T, 3> {
public:
    typedef T value_type;
    typedef std::size_t size_type;
    typedef lin_multi_span<T, 3> base_type;

    array_3d_indexer() : base_type()
    {
    }

    array_3d_indexer(T* const arr, const gpm_plugin_api_3d_memory_layout& layout) : base_type(
        arr, {
            layout.num_rows, layout.num_cols, layout.num_samples
        }, {layout.row_stride, layout.col_stride, layout.sample_stride})
    {
    }

    T const& operator()(std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k) const
    {
        return this->operator[]({i, j, k});
    }

    T& operator()(std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k)
    {
        return this->operator[]({i, j, k});
    }

    T const& get(std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k) const
    {
        return this->operator[]({i, j, k});
    }

    T& get(std::ptrdiff_t i, std::ptrdiff_t j, std::ptrdiff_t k)
    {
        return this->operator[]({i, j, k});
    }
    std::size_t num_rows() const
    {
        return this->dims[0];
    }

    std::size_t num_cols() const
    {
        return this->dims[1];
    }
    std::size_t num_samples() const
    {
        return this->dims[2];
    }
};

/**
 * C++ equivalent of C struct
 */
struct sediment_position_type {
    using index_type = int;
    std::string name;
    std::string id;
    index_type index_in_array{};
};

struct sediment_material_property_type {
    std::string name;
    std::string id;
    float diameter{};
    float grain_density{};
    float initial_porosity{};
    float final_porosity{};
    float initial_permeability{};
    float final_permeability{};
    float permeability_anisotropy{};
    float compaction{};
};

struct top_sediment_sea_parms {
    struct gpm_plugin_api_timespan time;
    array_2d_indexer<const float> top; // top surface z values
    array_3d_indexer<const float> sediments; /**< sediment values for active layer */
    array_3d_indexer<const float> erodibility; /**< erodibility of sediments, layout as sediments */
    lin_span<const float> transportibility; /**< transportability values for sediments*/
    array_3d_indexer<float> sediment_delta; /**< sediment values moved by process*/
    bool sediment_removed;
    /**< Has this process removed sediments, 0 if not removed, !0 if any sediment proportion has been removed. 0 is the same as a source*/
    array_2d_indexer<const float> sealevel; // sea level z values
};
/**
 * Convenience functions to generate C++ holders of C structs
 */
inline std::vector<std::string> make_attributes(const gpm_plugin_api_string_layout* attr_names, size_t num_attributes)
{
    std::vector<std::string> res;
    res.reserve(num_attributes);
    for (int i = 0; i < num_attributes; ++i) {
        res.emplace_back(attr_names[i].str, attr_names[i].str_length);
    }
    return res;
}

inline std::map<std::string, std::vector<Api::array_2d_indexer<float>>>
make_array_holders(const gpm_plugin_api_process_attribute_parms& attrs)
{
    std::map<std::string, std::vector<Api::array_2d_indexer<float>>> res;
    auto const_surf = attrs.surface_layout;
    const_surf.col_stride = 0;
    const_surf.row_stride = 0;
    for (int i = 0; i < attrs.num_attributes; ++i) {
        std::string name(attrs.attr_names[i].str, attrs.attr_names[i].str_length);
        std::vector<Api::array_2d_indexer<float>> holders;
        for (int j = 0; j < attrs.num_attr_array[i]; ++j) {
            auto tmp = Slb::Exploration::Gpm::Api::array_2d_indexer<float>(
                attrs.attributes[i][j], (attrs.is_constant[i][j] ? const_surf : attrs.surface_layout));
            holders.push_back(tmp);
        }
        res[name] = holders;
    }
    return res;
}

inline std::vector<sediment_position_type>
make_sediment_positions(gpm_plugin_api_sediment_position* seds, int num_seds)
{
    std::vector<sediment_position_type> res;
    for (int i = 0; i < num_seds; ++i) {
        sediment_position_type item;
        item.id = std::string(seds[i].id, seds[i].id_length);
        item.name = std::string(seds[i].name, seds[i].name_length);
        item.index_in_array = static_cast<sediment_position_type::index_type>(seds[i].index_in_sed_array);
        res.push_back(item);
    }
    return res;
}

inline std::vector<sediment_material_property_type>
make_sediment_materail_properties(gpm_plugin_api_sediment_material_properties* seds, int num_seds)
{
    std::vector<sediment_material_property_type> res;
    for (auto i = 0; i < num_seds; ++i) {
        sediment_material_property_type item;
        item.id = std::string(seds[i].id, seds[i].id_length);
        item.name = std::string(seds[i].name, seds[i].name_length);
        item.diameter = seds[i].diameter;
        item.grain_density = seds[i].grain_density;
        item.initial_porosity = seds[i].initial_porosity;
        item.final_porosity = seds[i].final_porosity;
        item.initial_permeability = seds[i].initial_permeability;
        item.final_permeability = seds[i].final_permeability;
        item.permeability_anisotropy = seds[i].permeability_anisotropy;
        item.compaction = seds[i].compaction;
        res.push_back(item);
    }
    return res;
}
inline top_sediment_sea_parms
make_sediment_transport_holder(gpm_plugin_api_process_with_top_sediment_sea_parms* parms)
{
    top_sediment_sea_parms res;
    res.time = parms->time;
    res.top = Slb::Exploration::Gpm::Api::array_2d_indexer<const float>(parms->top, parms->top_layout);
    res.sediments = Slb::Exploration::Gpm::Api::array_3d_indexer<const float>(parms->sediments, parms->sediment_layout);
    res.erodibility = Slb::Exploration::Gpm::Api::array_3d_indexer<const float>(
        parms->erodibility, parms->erodibility_layout);
    res.sediment_delta = Slb::Exploration::Gpm::Api::array_3d_indexer<float>(
        parms->sediment_delta, parms->sediment_delta_layout);
    res.transportibility = Slb::Exploration::Gpm::Api::lin_span<const float>(parms->transportibility, parms->num_sediments);
    res.sealevel= Slb::Exploration::Gpm::Api::array_2d_indexer<const float>(parms->sealevel, parms->sealevel_layout);
    return res;
}
}}}}
#endif
