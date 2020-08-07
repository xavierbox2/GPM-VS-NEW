#include "simple_plugin_process.h"
#include <string>
#include <algorithm>
#include "gpm_plugin_helpers.h"
#include <iostream>

using namespace Slb::Exploration::Gpm;
simple_plugin_process::simple_plugin_process( ) = default;

std::vector<std::pair<std::string, bool>> simple_plugin_process::list_needed_attribute_names( ) const
{
    // What we need, and is it an exact match
    std::vector<std::pair<std::string, bool>> needed_attributes = { {"TOP", true},{"POR", false} };
    return needed_attributes;
}

// This is the attribute we want to add, we'll call it offset
std::vector<simple_plugin_process::property_type> simple_plugin_process::list_wanted_attribute_names( ) const
{
    std::vector<property_type> write_attributes = { {"OFFSET", false} };
    return write_attributes;
}

// Here is where we can find all the attributes we want
// In this example we have TOP which is the geometry
// and POR, which is the proposity of the model
// access is the same as the
int simple_plugin_process::run_timestep( const attr_lookup_type& attributes, std::string* error )
{
//    // Here is the geometry access
//    // It would be the same for POR
//    // But for a real simulation:
//    // Here the coordinate converter should make these into a proper mapping from world and indexes
//    // Also the transformation from the GPM geometry to the unstructured model needs to happen
//    // Write the model, run the simulation
//
//    auto &top = attributes.at("TOP");
//    int nsurfaces = top.size();
//
//    //get the z's of each surface starting from the bootom up
//    for (int nsurface = 0; nsurface < top.size(); nsurface++)
//    {
//        // bootom up
//        auto &wit = top.at(nsurface);
//        //std::cout << "This is surface " << nsurface << std::endl;
//
//        int counter = 0;
//        for (int nrows = 0; nrows < wit.num_rows(); nrows++)
//        {
//            for (int ncols = 0; ncols < wit.num_cols(); ncols++)
//            {
//                float z = wit(nrows, ncols);//note that ncols is the second index. ncols varies the fastest    /********/
//                //std::cout << counter++ << " x  is stored " << " y is stored" << " z " << z << std::endl;
//            }
//        }
//    }
//
//    //I cannot be passing number by number, I will conver everything to ong arrays and pass them to the writer.
//    //can I get somehow that array or need to make a copy here?
//
//    auto& geom = attributes.at("TOP");
//    auto& por = attributes.at("POR");
//    for (int i = 0; i < geom.size(); ++i) {
//        auto& my_array_holder = geom.at(i);
//        auto& my_por = por.at(i);
//        for (auto j = 0; j < my_array_holder.num_rows(); ++j)
//        {
//            for (auto k = 0; k < my_array_holder.num_cols(); ++k)
//            {
//                auto z_val = my_array_holder(j, k);
//                auto por_val = my_por(j, k);
//            }
//        }
//    }
//
//    //geom.size() is the number of surfaces ?
//    //then, por.size() is also the number of surfaces?
//
//    //Ho do I iterate over ther surfaces?
//    //Get surface 1, get all the z values.
//    //Get surface 2, get all the z values?
//    //(etc)
//
//    ++_current_cycle;
//    //If you want to report something, do it in the string
    return 0;
}

int simple_plugin_process::update_results( std::map<std::string, std::vector<Api::array_2d_indexer<float>>>& attributes, std::string* error )
{
    // Here we would typically map back to the GPM geomtry to write the things we want back again
    // Those would typically be geometry changes and properties
    // Here we just do a float copy of the counter
    //auto& pres = attributes.at("OFFSET");
    //for (int i = 0; i < pres.size(); ++i) {
    //    auto& my_array_holder = pres.at(i);
    //    for (auto j = 0; j < my_array_holder.num_rows(); ++j) {
    //        for (auto k = 0; k < my_array_holder.num_cols(); ++k) {
    //            my_array_holder(j, k) = _current_cycle;
    //        }
    //    }
    //}
    return 0;
}