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
#ifndef gpm_simple_plugin_process_h
#define gpm_simple_plugin_process_h

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "gpm_plugin_helpers.h"
#include <map>
#include <vector>

#include "simple_classifier.h"

// This process is for demo only
// in essence it will ask for two properties, which is TOP and POR
// It will create an output property which is OFFSET
// It will then just call run_timestep, which in essence willl do nothing
// And when update_results are called, it will fill in an arbitrary offset, which will be the current cycle number
class simple_plugin_process {
public:
    using attr_lookup_type = std::map<std::string, std::vector<Slb::Exploration::Gpm::Api::array_2d_indexer<float>>>;

    struct property_type {
        std::string name;
        bool top_layer_only;
    };

    simple_plugin_process();
    std::string get_id() const;
    int read_parameters(const std::string& cs, std::string* err);
    std::vector<std::pair<std::string, bool>> get_needed_attributes() const;
    std::vector<property_type> get_wanted_attributes() const;
    int initialize_display_step(const attr_lookup_type& attributes, std::string* error);
    int run_timestep(const attr_lookup_type& attributes, std::string* error);
    int update_results(attr_lookup_type& attributes, std::string* error);
    void set_materials(const std::vector<Slb::Exploration::Gpm::Api::sediment_material_property_type>& vector);
    void set_array_size(const size_t num_rows, const size_t num_columns);
    void set_directory(const std::string& dir);
    void set_sediment_positions(const std::vector<Slb::Exploration::Gpm::Api::sediment_position_type>& seds);
    bool transport_sediments(Slb::Exploration::Gpm::Api::top_sediment_sea_parms& parms, std::string* error);
private:
    std::vector<Slb::Exploration::Gpm::Api::sediment_material_property_type> _sediments;
    std::vector<Slb::Exploration::Gpm::Api::sediment_position_type> _sediment_pos;
    std::map<std::string, std::string> _sed_name_to_id;
    std::map<std::string, std::string> _sed_id_to_name;
    simple_classifier _classify;
    std::string _dir_name;
};

#endif
