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

#include "simple_classifier.h"

#include <iterator>

simple_classifier::simple_classifier()
= default;

void simple_classifier::setup_classifier(const std::vector<mechanical_property>& classes)
{
    _mapper = classes;
}

void simple_classifier::setup_name_map(const std::map<std::string, std::string>& sed_id_to_name)
{
    _sed_id_to_name = sed_id_to_name;
}

float simple_classifier::find_distance(const std::vector<float>& ref,
                                       const std::vector<float>& xes)
{
    std::vector<float> diff;
    std::transform(ref.begin(), ref.end(), xes.begin(), std::back_inserter(diff), std::minus<float>());
    auto res = std::inner_product(diff.begin(), diff.end(), diff.begin(), 0.0F);
    return sqrt(res);
}

// All should be of same length
void simple_classifier::classify(
    const std::map<std::string, const Slb::Exploration::Gpm::Api::array_2d_indexer<float>*>& input)
{
    // Map up some stuff
    std::vector<std::pair<int, std::vector<const Slb::Exploration::Gpm::Api::array_2d_indexer<float>*>>> ordered;
    std::vector<std::pair<int, std::vector<float>>> percent;
    for (const auto& it : _mapper) {
        std::vector<const Slb::Exploration::Gpm::Api::array_2d_indexer<float>*> items;
        std::vector<float> parts;
        for (const auto& sed : it.sed_composition) {
            items.push_back(input.at(_sed_id_to_name.at(sed.sed_id)));
            parts.push_back(static_cast<float>(sed.percent));
        }
        ordered.emplace_back(it.class_id, items);
        percent.emplace_back(it.class_id, parts);
    }
    for (auto i = 0; i < _view.num_rows(); ++i) {
        for (auto j = 0; j < _view.num_cols(); ++j) {
            std::vector<std::vector<float>> parts;
            for (const auto& it : ordered) {
                std::vector<float> seds;
                for (const auto* sed_parts : it.second) {
                    seds.push_back((*sed_parts)(i, j));
                }
                parts.push_back(seds);
            }
            std::vector<float> dist_vec;
            for (int k = 0; k < percent.size(); ++k) {
                auto dist = find_distance(percent[k].second, parts[k]);
                dist_vec.push_back(dist);
            }
            auto min = static_cast<int>(std::distance(dist_vec.begin(),
                                                      std::min_element(dist_vec.begin(), dist_vec.end())));
            _view(i, j) = static_cast<float>(percent[min].first);
        }
    }
}

std::vector<std::string> simple_classifier::needed_sediment_names()
{
    std::vector<std::string> tmp;
    for (auto it : _mapper) {
        std::transform(it.sed_composition.begin(), it.sed_composition.end(), std::back_inserter(tmp),
                       [](const sediment_proportion& item) { return item.sed_id; });
    }
    std::vector<std::string> res;
    std::transform(tmp.begin(), tmp.end(), std::back_inserter(res),
                   [this](const std::string& it) { return this->_sed_id_to_name.at(it); });
    std::sort(res.begin(), res.end());
    const auto last = std::unique(res.begin(), res.end());
    res.erase(last, res.end());
    return res;
}

void simple_classifier::set_array_size(size_t num_rows, size_t num_columns)
{
    _holder.resize(num_rows * num_columns);
    _view = Slb::Exploration::Gpm::Api::array_2d_indexer<float>(_holder.data(), {
                                                                    num_rows, num_columns, (ptrdiff_t)num_columns, 1
                                                                });
}

const Slb::Exploration::Gpm::Api::array_2d_indexer<float>& simple_classifier::classified_result() const
{
    return _view;
}
