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

#include "simple_plugin_process.h"
#include "gpm_plugin_helpers.h"
#include <string>
#include <algorithm>

#include "simple_plugin_process_reader.h"

using namespace Slb::Exploration::Gpm;
simple_plugin_process::simple_plugin_process()
= default;

std::string simple_plugin_process::get_id() const
{
    return std::string("SIMPLE_PLUGIN");
}

int simple_plugin_process::read_parameters(const std::string& cs, std::string* err)
{

return  0;
    //simple_plugin_process_reader reader;
    //auto res = reader.read_file(cs, err);
    //// Lets read it up, and make some splash
    //if (res == 0) {
    //    _classify.setup_classifier(reader.get_params());
    //}
    //return res;
}

std::vector<std::pair<std::string, bool>> simple_plugin_process::get_needed_attributes() const
{
    // What we need, and is it an exact match
    std::vector<std::pair<std::string, bool>> needed_attributes = {{"TOP", false}, {"POR", false}, {"SEALEVEL", true}};
    // Now lets get all the sediments needed
    std::transform(_sediments.begin(), _sediments.end(), std::back_inserter(needed_attributes),
                   [](const Api::sediment_material_property_type& item)
                   {
                       return std::pair<std::string, bool>{item.name, true};
                   });
    return needed_attributes;
}

// This is the attribute we want to add, we'll call it offset
std::vector<simple_plugin_process::property_type> simple_plugin_process::get_wanted_attributes() const
{
    std::vector<property_type> write_attributes = {{"TOP", false}, {"CLASS_ID", true}};
    return write_attributes;
}

// Here is where we can find all the attributes we want
// In this example we have TOP which is the geometry
// and SEALEVEL, which is the current sealevel

int simple_plugin_process::initialize_display_step(const attr_lookup_type& attributes, std::string* error)
{
    // Here is the geometry access
    // It would be the same for POR
    // But for a real simulation:
    // Here the coordinate converter should make these into a proper mapping from world and indexes
    // Also the transformation from the GPM geometry to the unstructured model needs to happen
    // Write the model, run the simulation
    const auto& geom = attributes.at("TOP");
    const auto& sealevel = attributes.at("SEALEVEL");

    // Sea level is a top only type attribute, thus only one part for now.
    for (const auto& my_sealevel : sealevel) {
        for (auto j = 0; j < my_sealevel.num_rows(); ++j) {
            for (auto k = 0; k < my_sealevel.num_cols(); ++k) {
                auto sealevel_val = my_sealevel(j, k);
                // Initialize what is needed with the top geometry and sea level
            }
        }
    }
    for (const auto& my_array_holder : geom) {
        for (auto j = 0; j < my_array_holder.num_rows(); ++j) {
            for (auto k = 0; k < my_array_holder.num_cols(); ++k) {
                auto z_val = my_array_holder(j, k);
                // Initialize what is needed with the top geometry and sea level
            }
        }
    }

    //If you want to report something, do it in the string
    return 0;
}

// Here is where we can find all the attributes we want
// In this example we have TOP which is the geometry
// and POR, which is the proposity of the model
// access is the same as the 
int simple_plugin_process::run_timestep(const attr_lookup_type& attributes, std::string* error)
{
    // Here is the geometry access
    // It would be the same for POR
    // But for a real simulation:
    // Here the coordinate converter should make these into a proper mapping from world and indexes
    // Also the transformation from the GPM geometry to the unstructured model needs to happen
    // Write the model, run the simulation
    auto& geom = attributes.at("TOP");
    auto& por = attributes.at("POR");

    auto names = _classify.needed_sediment_names();
    for (int i = 0; i < geom.size(); ++i) {
        auto& my_array_holder = geom.at(i);
        auto& my_por = por.at(i);
        for (auto j = 0; j < my_array_holder.num_rows(); ++j) {
            for (auto k = 0; k < my_array_holder.num_cols(); ++k) {
                auto z_val = my_array_holder(j, k);
                auto por_val = my_por(j, k);

            }
        }
    }
    // Lets classify
    //
    std::map<std::string, const Api::array_2d_indexer<float>*> mapper;
    for (auto it : names) {
        mapper[it] = &attributes.at(it).back();
    }
    _classify.classify(mapper);
    //If you want to report something, do it in the string
    return 0;
}

int simple_plugin_process::update_results(std::map<std::string, std::vector<Api::array_2d_indexer<float>>>& attributes,
                                          std::string* error)
{
    // Here we would typically map back to the GPM geomtry to write the things we want back again
    // Those would typically be geometry changes and properties
    // Here we just do a float copy of the counter
    auto& pres = attributes.at("CLASS_ID");
    auto& my_array_holder = pres.back();
    auto classifi = _classify.classified_result();
    for (auto j = 0; j < my_array_holder.num_rows(); ++j) {
        for (auto k = 0; k < my_array_holder.num_cols(); ++k) {
            my_array_holder(j, k) = classifi(j, k);
        }
    }

    return 0;
}

void simple_plugin_process::set_materials(
    const std::vector<Api::sediment_material_property_type>& material_list)
{
    _sediments = material_list;
    for (auto id : material_list) {
        _sed_name_to_id[id.name] = id.id;
        _sed_id_to_name[id.id] = id.name;
    }
    _classify.setup_name_map(_sed_id_to_name);
    // Now we can map them to whatever
}

void simple_plugin_process::set_array_size(size_t num_rows, size_t num_columns)
{
    _classify.set_array_size(num_rows, num_columns);
}

void simple_plugin_process::set_directory(const std::string& dir)
{
    _dir_name = dir;
}

void simple_plugin_process::set_sediment_positions(const std::vector<Api::sediment_position_type>& seds)
{
    _sediment_pos = seds;
}

bool simple_plugin_process::transport_sediments(Api::top_sediment_sea_parms& parms, std::string* error)
{
    // Here we have the top surface, the active layer with all the sediments involved
    // For simplicity, lets juts add one meter of sediment 0
    //
    auto& out_sed(  parms.sediment_delta);
    for (auto j = 0; j < out_sed.num_rows(); ++j) {
        for (auto k = 0; k < out_sed.num_cols(); ++k) {
            out_sed(j, k, 0) = 1.0F;
        }
    }
    return true;
}