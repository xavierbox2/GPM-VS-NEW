// -- Schlumberger Private --
#ifndef gpm_plugin_api_plugin_description_h
#define gpm_plugin_api_plugin_description_h

#ifdef WIN32
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

    /*ids needed to find the most common properties*/

    static const char* geometry_id = "TOP";

    /* num_rows is along the local Y axis
     * num_cols is along the local X axis
     * The local x axis is pos[1]-pos[0]
     * The local Y axis is pos[3]-pos[0]
     */
    struct gpm_plugin_api_model_definition {
        size_t num_rows;
        size_t num_columns;
        float x_coordinates[4];
        float y_coordinates[4];
    };
    /* Sediment definition
     * with an id
     * name, typically SEDi etc
     * and the index in the column of the array
     */
    struct gpm_plugin_api_sediment_definition {
        const char*  id;
        size_t id_length;
        const char*  name;
        size_t name_length;
        ptrdiff_t index_in_sed_array;
    };
    /*
     * Describes the duration of this calculation
     */
    struct gpm_plugin_api_timespan {
        double start;
        double end;
    };
    enum gpm_plugin_api_loglevel { gpm_plugin_api_log_none = 0, gpm_plugin_api_log_important, gpm_plugin_api_log_normal, gpm_plugin_api_log_verbose, gpm_plugin_api_log_debug, gpm_plugin_api_log_trace };
    /* where to put the error message if happening
     * message_length is how much the callee has filled in
     * message_array_length is the max length that can be used
     */
    struct gpm_plugin_api_message_definition {
        char* message;
        size_t message_length;
        size_t message_array_length;
        int log_level;
    };
    struct gpm_plugin_api_string_layout {
        char* str;
        size_t str_length;
    };
    struct gpm_plugin_api_2d_memory_layout {
        size_t num_rows;
        size_t num_cols;
        ptrdiff_t row_stride;
        ptrdiff_t col_stride;
    };

    struct gpm_plugin_api_3d_memory_layout {
        size_t num_rows;
        size_t num_cols;
        size_t num_samples;
        ptrdiff_t row_stride;
        ptrdiff_t col_stride;
        ptrdiff_t sample_stride;
    };
    struct gpm_plugin_api_process_with_top_sediment_sea_parms {
        gpm_plugin_api_timespan time;
        float const *  top; // top surface z values
        gpm_plugin_api_2d_memory_layout top_layout;
        float*  sediments;  // sediment values for top layer
        gpm_plugin_api_3d_memory_layout sediment_layout;
        float const * sealevel;  // sea level z values
        gpm_plugin_api_2d_memory_layout sealevel_layout;
        gpm_plugin_api_message_definition error;
    };
    struct gpm_plugin_api_process_attribute_parms {
        gpm_plugin_api_timespan time;
        float*** attributes;
        uint8_t** is_constant; /* is it a constant or a surface*/
        size_t* num_attr_array;
        gpm_plugin_api_string_layout* attr_names;
        size_t num_attributes; /* same for attr_names and attributes*/
        gpm_plugin_api_2d_memory_layout surface_layout;
        gpm_plugin_api_message_definition error;
    };

    /* The API for some base type of actions*/
    /* Called in this order approximately now*/
    typedef void* (*create_plugin_func)();
    typedef void( *set_model_extents_func )(void* handle, const gpm_plugin_api_model_definition* const model);
    typedef void( *current_install_dir_func )(void* handle, const char* const parameters_file_name, int name_len);
    typedef void( *set_sediment_func )(void* handle, gpm_plugin_api_sediment_definition* seds, int num_seds);
    typedef int( *read_parameters_func )(void* handle, const char* const parameters_file_name, int name_len, gpm_plugin_api_message_definition* error_msg);

    typedef int( *get_needed_model_attributes_func )(void* handle, int num, gpm_plugin_api_string_layout* attributes, int* needed, gpm_plugin_api_message_definition* error_msg);
    typedef int( *get_write_model_attribute_num_func )(void* handle);
    typedef void( *get_write_model_attribute_sizes_func )(void* handle, int* attr_length, int num);
    typedef void( *get_write_model_attributes_func )(void* handle, gpm_plugin_api_string_layout* attributes, int* top_only_attr, int num);

    typedef void( *initialize_display_func )(void* handle, double time);
    typedef int( *process_top_sediment_sea_timestep_func )(void* handle, gpm_plugin_api_process_with_top_sediment_sea_parms* parms);

    // Which multitude of display time steps should we call out for?
    typedef int( *process_model_multiple_of_timestep_func )(void* handle, double display_timestep);
    typedef int( *process_model_timestep_func )(void* handle, gpm_plugin_api_process_attribute_parms* parms);
    typedef int( *update_attributes_timestep_func )(void* handle, gpm_plugin_api_process_attribute_parms* parms);

    typedef void( *delete_plugin_func )(void* handle);

    DLLEXPORT void* gpm_plugin_api_create_plugin_handle( );
    DLLEXPORT void gpm_plugin_api_delete_plugin_handle( void* handle );
    /* Initialize with given parameters, and return != 0 if something went wrong. Fill in the error message, and the length of the error */
    DLLEXPORT int gpm_plugin_api_read_parameters( void* handle, const char* const parameters_file_name, int name_len, gpm_plugin_api_message_definition* error_msg );
    DLLEXPORT void gpm_plugin_api_current_install_directory( void* handle, const char* const install_dir_name, int name_len );
    DLLEXPORT void gpm_plugin_api_set_model_extents( void* handle, const gpm_plugin_api_model_definition* const model );
    DLLEXPORT void gpm_plugin_api_set_sediments( void* handle, gpm_plugin_api_sediment_definition* seds, int num_seds );

    /* What attributes does the process neeed
     * All found return 0
     * If not, return != 0
     */
    DLLEXPORT int gpm_plugin_api_get_needed_model_attributes( void* handle, int num, gpm_plugin_api_string_layout* attributes, int* needed, gpm_plugin_api_message_definition* error_msg );
    /* What attributes does the process write back*/
    DLLEXPORT int gpm_plugin_api_get_write_model_attribute_num( void* handle );
    DLLEXPORT void gpm_plugin_api_get_write_model_attribute_sizes( void* handle, int* attr_length, int num );
    DLLEXPORT void gpm_plugin_api_get_write_model_attributes( void* handle, gpm_plugin_api_string_layout* attributes, int* top_only_attr, int num );

    DLLEXPORT void gpm_plugin_api_initialize_display_step( void* handle, double time );
    DLLEXPORT int gpm_plugin_api_process_top_sediment_sea_timestep( void* handle, gpm_plugin_api_process_with_top_sediment_sea_parms* parms );

    /* The multitude of timesteps to call out to the two other functions below*/
    DLLEXPORT int gpm_plugin_api_process_model_multiple_of_timestep( void* handle, double display_timestep );
    DLLEXPORT int gpm_plugin_api_process_model_timestep( void* handle, gpm_plugin_api_process_attribute_parms* parms );
    DLLEXPORT int gpm_plugin_api_update_attributes_timestep( void* handle, gpm_plugin_api_process_attribute_parms* parms );

#ifdef __cplusplus
}
#endif

#endif