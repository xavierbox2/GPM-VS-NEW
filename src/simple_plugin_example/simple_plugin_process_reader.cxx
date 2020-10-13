///** Copyright (c) Schlumberger 2020. All rights reserved.
// *  GPM simulator plugin API
// *
// * Disclaimer
// * Use of this product is governed by the License Agreement. Schlumberger
// * makes no warranties, express, implied, or statutory, with respect to the
// * product described herein and disclaims without limitation any warranties of
// * merchantability or fitness for a particular purpose. Schlumberger reserves the
// * right to revise the information in this manual at any time without notice.
//*/
//
///** Warranty Disclaimer:
//  *
//  * The APIs exposed in this package are not subject to restrictions regarding breaking changes.
//  * Although it is Schlumberger Information Solutions' ambition that these APIs will continue to work as they originally worked
//  * and any existing applications that use the APIs will continue to work without changes, it is expected that the process of
//  * commercialization and future architectural changes will necessitate incompatible changes.
//*/
//
#include "simple_plugin_process_reader.h"
////#include <rapidjson/istreamwrapper.h>
////#include <rapidjson/pointer.h>
////#include <rapidjson/error/en.h>
//#include <fstream>
//
//simple_plugin_process_reader::simple_plugin_process_reader()
//= default;
//
//int simple_plugin_process_reader::read_file(const std::string& file, std::string* err_log)
//{
//    int retval = 1;
//    //rapidjson::Document d;
//    //std::ifstream ifd(file);
//    //rapidjson::IStreamWrapper isdw(ifd);
//    //if (d.ParseStream(isdw).HasParseError()) {
//    //    // the input is not a valid JSON.
//    //    // ...
//    //    *err_log = std::string("Error(offset ") + std::to_string(static_cast<unsigned>(d.GetErrorOffset())) +
//    //        rapidjson::GetParseError_En(d.GetParseError());
//    //    retval += 2;
//    //}
//    //retval = check_file(d);
//    return retval;
//}
//
//int simple_plugin_process_reader::check_file(const rapidjson::Document& doc)
//{
//    int retval = 1;
//    if (doc.HasMember("PARAMETERS") && doc["PARAMETERS"].IsObject()) {
//        const auto& params(doc["PARAMETERS"]);
//        // Find the transforms for sediments
//        std::vector<simple_classifier::mechanical_property> res;
//        if (params.HasMember("SedimentTransformSource") && params["SedimentTransformSource"].IsArray()) {
//            const auto sources_array = params["SedimentTransformSource"].GetArray();
//            for (rapidjson::Value::ConstValueIterator itr_row = sources_array.Begin(); itr_row != sources_array.End();
//                 ++itr_row) {
//                const auto source_ptr = rapidjson::Pointer(itr_row->Get<std::string>());
//                const auto source = source_ptr.Get(doc)->GetObject();
//                const auto id = source["SOURCE_ID"].GetInt();
//                const auto src_params = source["PARAMETERS"].GetObject();
//                // These could be functions as well
//                const auto youngs = src_params["YoungsModulus"].Get<double>();
//                const auto shear = src_params["ShearModulus"].Get<double>();
//                const auto src_sed_distro = src_params["SedimentComposition"].GetArray();
//                simple_classifier::mechanical_property tmp;
//                tmp.class_id = id;
//                tmp.youngs = youngs;
//                tmp.shear = shear;
//                for (rapidjson::Value::ConstValueIterator sed_row = src_sed_distro.Begin(); sed_row != src_sed_distro.
//                     End(); ++sed_row) {
//                    const auto sed_distro_ptr = rapidjson::Pointer(sed_row->Get<std::string>());
//                    const auto sed_distro = sed_distro_ptr.Get(doc)->GetObject();
//                    const auto sed_id = sed_distro["SEDIMENT_ID"].Get<std::string>();
//                    const auto& sed_params(sed_distro["PARAMETERS"]);
//                    const auto percent = sed_params["percent"].GetDouble();
//                    simple_classifier::sediment_proportion sed_tmp;
//                    sed_tmp.sed_id = sed_id;
//                    sed_tmp.percent = percent;
//                    tmp.sed_composition.push_back(sed_tmp);
//                }
//                res.push_back(tmp);
//            }
//        }
//        _params = res;
//        return 0;
//    }
//    return retval;
//}
//
//std::vector<simple_classifier::mechanical_property> simple_plugin_process_reader::get_params() const
//{
//    return _params;
//}
