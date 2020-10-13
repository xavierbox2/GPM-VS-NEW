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
#ifndef SIMPLE_CLASSIFIER_H
#define SIMPLE_CLASSIFIER_H


#include "gpm_plugin_helpers.h"

#include <map>
#include <string>
#include <vector>

class simple_classifier {
public:
    struct sediment_proportion {
        std::string sed_id;
        double percent;
    };

    struct mechanical_property {
        int class_id;
        double youngs;
        double shear;
        std::vector<sediment_proportion> sed_composition;
    };

    simple_classifier();
    void setup_classifier(const std::vector<mechanical_property>& classes);
    void setup_name_map(const std::map<std::string, std::string>& sed_id_to_name);
    float find_distance(const std::vector<float>& ref, const std::vector<float>& xes);
    void classify(const std::map<std::string, const Slb::Exploration::Gpm::Api::array_2d_indexer<float>*>& input);
    std::vector<std::string> needed_sediment_names();
    void set_array_size(size_t size, size_t num_columns);
    const Slb::Exploration::Gpm::Api::array_2d_indexer<float>& classified_result() const;
private:
    std::vector<mechanical_property> _mapper;
    std::map<std::string, std::string> _sed_id_to_name;
    Slb::Exploration::Gpm::Api::array_2d_indexer<float> _view;
    std::vector<float> _holder;
};


#endif
