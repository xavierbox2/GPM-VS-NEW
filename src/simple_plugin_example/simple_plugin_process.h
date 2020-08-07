#ifndef simple_plugin_process_h
#define simple_plugin_process_h

#include <string>
#include <vector>
#include <map>
#include <functional>

#include "gpm_plugin_helpers.h"
#include <map>
#include <vector>

class simple_plugin_process {
public:
    using attr_lookup_type = std::map<std::string, std::vector<Slb::Exploration::Gpm::Api::array_2d_indexer<float>>>;

    struct property_type {
        std::string name;
        bool top_layer_only;
    };

    simple_plugin_process( );
    std::vector<std::pair<std::string, bool>> list_needed_attribute_names( ) const;
    std::vector<property_type> list_wanted_attribute_names( ) const;
    int run_timestep( const attr_lookup_type& attributes, std::string* error );
    int update_results( attr_lookup_type& attributes, std::string* error );

private:
    int _current_cycle{};
};

#endif
