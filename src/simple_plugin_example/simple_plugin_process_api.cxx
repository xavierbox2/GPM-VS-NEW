#include "simple_plugin_process_api.h"
#include "simple_plugin_process.h"
#include "gpm_plugin_description.h"
#include "gpm_plugin_helpers.h"

#include <numeric>
#include <string>
#include <memory>
#include <map>
#include <sstream>
#include <functional>
#include "GpmVsCoupler.h"

namespace {
    class sediment_holder
    {
    public:
        std::string name;
        std::string id;
        int index_in_array{};
    };

    class process_wrapper
    {
    public:

        std::shared_ptr<gpm_visage_link> process;
        std::shared_ptr<IConfiguration> config;
        shared_ptr<IMechanicalPropertiesInitializer> props_model;
    };
}

extern "C" DLLEXPORT void* gpm_plugin_api_create_plugin_handle( )
{
    auto ptr = new process_wrapper( );
    
    ptr->config = std::make_shared< DefaultConfiguration >( );
    
    //ptr->props_model = std::make_shared< MechPropertiesEffectiveMedium >( );
    ptr->props_model = std::make_shared< MechPropertiesDVT >( );

    //MechPropertiesDVT
    //MechPropertiesEffectiveMedium
    ptr->process = std::make_shared< gpm_visage_link>( ptr->config, ptr->props_model );

    return ptr;
}

process_wrapper* get_process_wrapper( void* handle )
{
    return static_cast<process_wrapper*>(handle);
}

extern "C" DLLEXPORT void gpm_plugin_api_delete_plugin_handle( void* handle )
{
    delete get_process_wrapper( handle );
}

// Read your input file, skip for now
extern "C" DLLEXPORT int gpm_plugin_api_read_parameters( void* handle, const char* const parameters_file_name, int name_len, gpm_plugin_api_message_definition * error_msg )
{
    cout << "Input file: " << parameters_file_name << endl;

    int return_code = 0;
    try
    {
        std::ifstream input_file( parameters_file_name );
        stringstream buffer;
        buffer << input_file.rdbuf( );
        input_file.close( );

        cout << "---------------------------------" << endl;
        cout << buffer.str( ) << endl;
        cout << "---------------------------------" << endl;
        auto ptr = get_process_wrapper( handle );
        if(!ptr->process->process_ui( buffer.str( ) ))
        {
            return_code = 1;
        }

    }
    catch(exception & e)
    {
        error_msg->message_length = strlen( e.what( ) );//, error_msg->message_array_length );
        sprintf( error_msg->message, "%s", e.what( ) );
        return_code = 1;
    }

    return return_code;
}
// Get the install directory, in case you have a simulator to call from there
extern "C" DLLEXPORT void gpm_plugin_api_current_install_directory( void* handle, const char* const dir_name, int name_len )
{
    std::cout << "Install directory: " << dir_name << endl;
    ;
    ;
    ;
}
// Setup the model extents so a new coordinate system can be modelled and used
extern "C" DLLEXPORT void gpm_plugin_api_set_model_extents( void* handle, const gpm_plugin_api_model_definition* const model )
{
    auto ptr = get_process_wrapper( handle );
    ptr->process->initialize_model_extents( model );
}
// setup the sediment definitions
extern "C" DLLEXPORT void gpm_plugin_api_set_sediments( void* handle, gpm_plugin_api_sediment_definition * seds, int num_seds )
{
    cout << "We have " << num_seds << " sediments definitions " << endl;
    for(auto n : IntRange( 0, num_seds ))
    {
        //const char*  id;
        //size_t id_length;
        //const char*  name;
        //size_t name_length;
        //ptrdiff_t index_in_sed_array;
        gpm_plugin_api_sediment_definition* ptr = &seds[n];
        cout << "id " << ptr->id << endl;
        cout << "name " << ptr->name << endl;
        cout << "index " << ptr->index_in_sed_array << endl;
        ;
        ;
    }
}
// Do whatever you need to for init of the display steps
extern "C" DLLEXPORT void gpm_plugin_api_initialize_display_step( void* handle, double time )
{
    (void)get_process_wrapper( handle );
}
// The function that will be called at each small increment
extern "C" DLLEXPORT int gpm_plugin_api_process_top_sediment_sea_timestep( void* handle, gpm_plugin_api_process_with_top_sediment_sea_parms * parms )
{
    return 0;
}

// Which attributes do you need to have, are they available?
//num = number of all the attributes that exist
extern "C" DLLEXPORT int gpm_plugin_api_get_needed_model_attributes( void* handle, int num, gpm_plugin_api_string_layout * attributes, int* needed, gpm_plugin_api_message_definition * error_msg )
{
    int res = 0;
    const auto ptr = get_process_wrapper( handle );

    // This si convenience function for making all of this attributes simple strings
    auto all_attrs = Slb::Exploration::Gpm::Api::make_attributes( attributes, num );

    // These attributes are the ones we need, TOP and POR
    auto needed_attributes = ptr->process->list_needed_attribute_names( all_attrs );

    // Here we have a full match, so we go straight to it
    std::vector<std::string> missing;
    std::vector<int> indexes;
    for(const auto& need_it : needed_attributes) {
        auto it = std::find( all_attrs.begin( ), all_attrs.end( ), need_it.first );
        if(it != all_attrs.end( ))
        {
            indexes.push_back( std::distance( all_attrs.begin( ), it ) );
        }
        else {
            missing.push_back( need_it.first );
        }
    }
    // Tell the ones we need back to the runner
    if(missing.empty( )) {
        for(auto loc_index : indexes) {
            needed[loc_index] = 1;
        }
    }
    else {
        // Report the ones that are not there that we need.
        const auto all_missed = std::accumulate( missing.begin( ), missing.end( ), std::string( ),
                                                 []( const std::string& init, const std::string& next )
                                                 {
                                                     if(init.empty( )) {
                                                         return next;
                                                     }
                                                     return init + std::string( ", " ) + next;
                                                 } );
        error_msg->log_level = gpm_plugin_api_log_normal;
        std::string tmp = std::string( "Attributes missed: " ) + all_missed + "\n";
        std::copy( tmp.begin( ), tmp.end( ), error_msg->message );
        error_msg->message_length = tmp.size( );
        res = 1;
    }
    return res;
}

/* What attributes does the process write back*/
extern "C" DLLEXPORT int gpm_plugin_api_get_write_model_attribute_num( void* handle )
{
    const auto ptr = get_process_wrapper( handle );
    const auto write_attributes = ptr->process->list_wanted_attribute_names( );
    return  (int)write_attributes.size( );
}
// These are just the length of the attributes strings we need
extern "C" DLLEXPORT void gpm_plugin_api_get_write_model_attribute_sizes( void* handle, int* attr_length, int num )
{
    int i = 0;
    const auto ptr = get_process_wrapper( handle );
    auto write_attributes = ptr->process->list_wanted_attribute_names( );
    for(const auto& it : write_attributes) {
        attr_length[i] = (int)it.name.size( );
        ++i;
    }
}
// now we ask for the attribute string itself
extern "C" DLLEXPORT void gpm_plugin_api_get_write_model_attributes( void* handle, gpm_plugin_api_string_layout * attributes, int* top_only_attr, int num )
{
    int i = 0;
    const auto ptr = get_process_wrapper( handle );
    auto write_attributes = ptr->process->list_wanted_attribute_names( );
    for(auto it : write_attributes) {
        std::copy( it.name.begin( ), it.name.end( ), attributes[i].str );
        attributes[i].str_length = it.name.size( );
        top_only_attr[i] = it.top_layer_only ? 1 : 0;
        ++i;
    }
}

// Run a display time step with the attributes we said we need
// Typically means that we take the geometry and transform to an unstractured grid
// Run the simulation needed
// Keep the results for the next call which is gpm_plugin_api_update_attributes_timestep
// There all the results get updated
extern "C" DLLEXPORT int gpm_plugin_api_process_model_timestep( void* handle, gpm_plugin_api_process_attribute_parms * params )
{
    std::shared_ptr<gpm_visage_link>   ptr = get_process_wrapper( handle )->process;
    const auto attrs = Slb::Exploration::Gpm::Api::make_array_holders( *params );
    std::string log;

    const int res = ptr->run_timestep( attrs, log, params->time );
    return res;

    //ptr->update_geometry_from_top_property(attrs);
    //struct gpm_plugin_api_process_attribute_parms {
    //	gpm_plugin_api_timespan time;
    //	float*** attributes;
    //	uint8_t** is_constant; /* is it a constant or a surface*/
    //	size_t* num_attr_array;
    //	gpm_plugin_api_string_layout* attr_names;
    //	size_t num_attributes; /* same for attr_names and attributes*/
    //	gpm_plugin_api_2d_memory_layout surface_layout;
    //	gpm_plugin_api_message_definition error;
    //};
    //ptr->debug_write_surface_files( ptr->_time_step, n_surfaces );
    ////float**** ptr = parms->attributes;
    ////float **s ;
    //auto end = parms->time.end;
 //   // Write out the files
 //   // Run the process
 //   const auto attrs = Slb::Exploration::Gpm::Api::make_array_holders(*parms);
    //const auto ptr = get_process_wrapper(handle);
    //std::string log;
    //// Just call the wrapper with a couple of things
 //   const int res = ptr->process->run_timestep(attrs,&log);
    //parms->error.log_level = gpm_plugin_api_log_normal;
    //if (!log.empty()) {
    //	std::string tmp = log + "\n";
    //	std::copy(tmp.begin(), tmp.end(), parms->error.message);
    //	parms->error.message_length = tmp.size();
    //}
}
//Here, we take the results from the Geomechanics simulation and copy the results into the gpm arrays.
extern "C" DLLEXPORT int gpm_plugin_api_update_attributes_timestep( void* handle, gpm_plugin_api_process_attribute_parms * parms )
{
    auto attrs = Slb::Exploration::Gpm::Api::make_array_holders( *parms );
    std::map<std::string, std::vector<Slb::Exploration::Gpm::Api::array_2d_indexer<float>>> ATT2 = Slb::Exploration::Gpm::Api::make_array_holders( *parms );

    const auto ptr = get_process_wrapper( handle );

    std::string log = "";
    const int res = ptr->process->update_results( attrs, log );
    parms->error.log_level = gpm_plugin_api_log_normal;
    if(!log.empty( )) {
        std::string tmp = log + "\n";
        std::copy( tmp.begin( ), tmp.end( ), parms->error.message );
        parms->error.message_length = tmp.size( );
    }

    return res;

    //parms->error.log_level = gpm_plugin_api_log_normal;
    //if (!log.empty()) {
    //	std::string tmp = log + "\n";
    //	std::copy(tmp.begin(), tmp.end(), parms->error.message);
    //	parms->error.message_length = tmp.size();
    //}

    //const int res = ptr->process->update_results(attrs, &log);
    //parms->error.log_level = gpm_plugin_api_log_normal;
    //if (!log.empty())
    //{
    //	std::string tmp = log + "\n";
    //	std::copy(tmp.begin(), tmp.end(), parms->error.message);
    //	parms->error.message_length = tmp.size();
    //}
 //   return res;
}