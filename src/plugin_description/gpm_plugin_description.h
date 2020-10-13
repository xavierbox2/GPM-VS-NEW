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

/** @file
 *
 *@brief
 * This file describes the parts that consist of the GPM public API
 * The file consists of the following parts:
 * - String identifiers for attributes
 * - Structs for holding data participating in the API
 * - API functions
 *@warning There should be no memory pointers that are allocated on one side and assumed to be freed on the other side of the API
 */

/** @var id names needed to find the most common properties */

    static const char* api_version_id = "0.1.0.0";
    static const char* geometry_id = "TOP";
    static const char* sealevel_id = "SEALEVEL";
    static const char* sediment_prefix_id = "SED";
    static const char* porosity_id = "POR";
    static const char* permeability_vertical = "PERMEABILITY_VERT";
    static const char* permeability_horizontal = "PERMEABILITY_HORI";

    /**
     * String memory layout.
     * Holds the pointer to the memory and the length of the string
     * The string does not need to be 0 terminated
     */
    struct gpm_plugin_api_string_layout {
        char* str;
        size_t str_length;
    };

    /**
     * Data memory layout.
     * Describes the logical 2d memory layout of a physical 1d array.
     * num_rows and num_cols describe the number of samples in row/col direction
     * Typically row_stride is the number of columns in the 2d array
     * and col_stride is 1
     */
    struct gpm_plugin_api_2d_memory_layout {
        size_t num_rows;
        size_t num_cols;
        ptrdiff_t row_stride;
        ptrdiff_t col_stride;
    };

    /**
    * Data memory layout.
    * Describes the logical 3d memory layout of a physical 1d array.
    * num_rows, num_cols and num_samples describe the number of samples in row/col/sample direction
    * Typically row_stride is the num_cols*num_samples in the 2d array,
    * col_stride is num_samples and sample_stride is 1
    */
    struct gpm_plugin_api_3d_memory_layout {
        size_t num_rows;
        size_t num_cols;
        size_t num_samples;
        ptrdiff_t row_stride;
        ptrdiff_t col_stride;
        ptrdiff_t sample_stride;
    };

    /**
     * Model definition
     * The coordinates define the position in x y world coordinates
     * The row cols define the number of samples along the given axis
     */
    struct gpm_plugin_api_model_definition {
        size_t num_rows; /**< num_rows is along the local Y axis */
        size_t num_columns; /**< num_cols is along the local X axis */
        float x_coordinates[4]; /**< The local x axis is pos[1]-pos[0] */
        float y_coordinates[4]; /**< The local Y axis is pos[3]-pos[0] */
    };

    /** Sediment definition
     * with an id
     * name, typically SEDi etc
     * and the index in the column of the array
     */
    struct gpm_plugin_api_sediment_position {
        const char* id;
        size_t id_length;
        const char* name;
        size_t name_length;
        ptrdiff_t index_in_sed_array;
    };

    /** Sediment properties
    * with an id
    * name, typically SEDi etc
    * things like diameter, porosity etc
    */
    struct gpm_plugin_api_sediment_material_properties {
        const char* id;
        size_t id_length;
        const char* name;
        size_t name_length;
        float diameter;
        float grain_density;
        float initial_porosity;
        float final_porosity;
        float initial_permeability;
        float final_permeability;
        float permeability_anisotropy;
        float compaction;
    };

    /**
     * Timespan description
     * start <= end along the timeline
     */
    struct gpm_plugin_api_timespan {
        double start;
        double end;
    };

    enum gpm_plugin_api_loglevel {
        gpm_plugin_api_log_none = 0,
        gpm_plugin_api_log_important,
        gpm_plugin_api_log_normal,
        gpm_plugin_api_log_verbose,
        gpm_plugin_api_log_debug,
        gpm_plugin_api_log_trace
    };

    /**
     * Message holder if needed.
     * The message has a predefined max length that can not be exceeded
     * message_length is how much the callee has filled in
     * message_array_length is the max length that can be used
     * log_level is to tell how important the message is
     */
    struct gpm_plugin_api_message_definition {
        char* message;
        size_t message_length;
        size_t message_array_length;
        int log_level;
    };

    /**
     * The structure of the sediments follow the index regime sent down by gpm_plugin_api_sediment_position
     * The sediment indexes in sample direction is the same for sediments, erodibility and transportability
     */
    struct gpm_plugin_api_process_with_top_sediment_sea_parms {
        struct gpm_plugin_api_timespan time;
        float const* top; // top surface z values
        struct gpm_plugin_api_2d_memory_layout top_layout;
        float* sediments; /**< sediment values for active layer */
        struct gpm_plugin_api_3d_memory_layout sediment_layout;
        float* erodibility; /**< erodibility of sediments, layout as sediments */
        struct gpm_plugin_api_3d_memory_layout erodibility_layout;
        float* transportibility; /**< transportability values for sediments*/
        size_t num_sediments; /**< number of sediments in active layer*/
        float* sediment_delta; /**< sediment values moved by process*/
        struct gpm_plugin_api_3d_memory_layout sediment_delta_layout;
        int sediment_removed;
        /**< Has this process removed sediments, 0 if not removed, !0 if any sediment proportion has been removed. 0 is the same as a source*/
        float const* sealevel; // sea level z values
        struct gpm_plugin_api_2d_memory_layout sealevel_layout;
        struct gpm_plugin_api_message_definition error;
    };

    /**
     * The class holds the timespan in question, the attributes that the process needs
     * Typically used at start of display time step with start time equal to end time
     * and end of display time step, where timespan is the actual time span of this display step
     */
    struct gpm_plugin_api_process_attribute_parms {
        struct gpm_plugin_api_timespan time;
        float*** attributes;
        uint8_t** is_constant; /* is it a constant or a surface*/
        size_t* num_attr_array;
        struct gpm_plugin_api_string_layout* attr_names;
        size_t num_attributes; /* same for attr_names and attributes*/
        struct gpm_plugin_api_2d_memory_layout surface_layout;
        struct gpm_plugin_api_message_definition error;
    };

    /* The API for some base type of actions*/
    /* Called in this order approximately now*/

    /**
    @brief Create the plugin handle.
    @param none
    */
    DLLEXPORT void* gpm_plugin_api_create_plugin_handle( );

    /**
    @brief Delete the plugin handle.
    @param handle -  the \a handle
    */
    DLLEXPORT void gpm_plugin_api_delete_plugin_handle( void* handle );

    /**
     @brief Find the plugin identifier string length
     @param handle -  the \a handle
     */
    DLLEXPORT int gpm_plugin_api_get_plugin_id_length( void* handle );

    /**
     @brief Find the plugin identifier
     @param handle -  the \a handle
     @param name - the name to be returned
     @param error_msg - the \a errormessage
     */
    DLLEXPORT int gpm_plugin_api_get_plugin_id( void* handle, struct gpm_plugin_api_string_layout* name );

    /**
    @brief Initialize with given parameters, and return != 0 if something went wrong. Fill in the error message, and the length of the error.
    @param handle - the \a handle
    @param parameters_file_name - the \a parameter \a filename
    @param name_len - the \a length of the error message
    @param error_msg - the \a errormessage
    */
    DLLEXPORT int gpm_plugin_api_read_parameters( void* handle, const char* const parameters_file_name, int name_len,
                                                  struct gpm_plugin_api_message_definition* error_msg );

     /**
     @brief Set the install directory.
     @param handle - the \a handle
     @param install_dir_name - the \a install \a directory
     @param name_len - the \a length of install directory name
     */
    DLLEXPORT void gpm_plugin_api_current_install_directory( void* handle, const char* const install_dir_name, int name_len );

    /**
    @brief Set the model size (extent).
    @param handle - the \a handle
    @param model - the \a model
    */
    DLLEXPORT void
        gpm_plugin_api_set_model_extents( void* handle, const struct gpm_plugin_api_model_definition* const model );

        /**
        @brief Set the sediment properties.
        @param handle - the \a handle
        @param seds - the \a sediments
        @param num_seds - the number of \a sediments
        */
    DLLEXPORT void gpm_plugin_api_set_sediments( void* handle, struct gpm_plugin_api_sediment_position* seds, int num_seds );

    /**
    @brief Set the sediment material properties.
    @param handle - the \a handle
    @param seds - the \a sediment  \a material  \a properties
    @param num_seds - the number of \a sediment  \a material  \a properties
    */
    DLLEXPORT void gpm_plugin_api_set_sediment_material_properties( void* handle,
                                                                    struct gpm_plugin_api_sediment_material_properties* seds,
                                                                    int num_seds );

     /**
     @brief What attributes does the process neeed.
     @brief All found return 0 -if not, return != 0.
     @param handle - the \a handle
     @param num - the \a number  of attributes
     @param attributes - the \a attributes
     @param needed - the \a needed  attributes
     @param error_msg - the \a errormessage
     */
    DLLEXPORT int gpm_plugin_api_get_needed_model_attributes( void* handle, int num,
                                                              struct gpm_plugin_api_string_layout* attributes, int* needed,
                                                              struct gpm_plugin_api_message_definition* error_msg );


     /**
     @brief What attributes does the process write back - return number of attributes
     @param handle -  the \a handle
     */
    DLLEXPORT int gpm_plugin_api_get_write_model_attribute_num( void* handle );

    /**
    @brief What attributes does the process write back - get size of attributes
    @param handle -  the \a handle
    @param attr_length -  the \a attribute-size
    */
    DLLEXPORT void gpm_plugin_api_get_write_model_attribute_sizes( void* handle, int* attr_length, int num );

    /**
    @brief What attributes does the process write back - get attributes ids and top only indx
    @param handle -  the \a handle
    @param attributes -  the \a attribute \a ids
    @param top_only_attr -  the \a top_only_attr
    */
    DLLEXPORT void gpm_plugin_api_get_write_model_attributes( void* handle, struct gpm_plugin_api_string_layout* attributes,
                                                              int* top_only_attr, int num );

     /**
     @brief initialize display step at time value
     Model state of the attributes will be sent down at this point to initialize model in process if needed
     @param handle -  the \a handle
     @param parms -  \a process \a model params where the start and end time is normally equal
     */
    DLLEXPORT int
        gpm_plugin_api_initialize_display_step( void* handle, struct gpm_plugin_api_process_attribute_parms* parms );

        /**
        @brief called for every method time increment -get top sediment
        @param handle -  the \a handle
        @param parms -  \a top sediment params
        */
    DLLEXPORT int gpm_plugin_api_process_top_sediment_sea_timestep( void* handle,
                                                                    struct gpm_plugin_api_process_with_top_sediment_sea_parms
                                                                    * parms );

     /**
     @brief The multitude of timesteps to call out to the two other functions below
     @param handle -  the \a handle
     @param display_timestep -  \a display \a timestep
     */
    DLLEXPORT int gpm_plugin_api_process_model_multiple_of_timestep( void* handle, double display_timestep );

    /**
    @brief write process model attributes for timestep
    @param handle -  the \a handle
    @param parms -  \a process \a model params
    */
    DLLEXPORT int gpm_plugin_api_process_model_timestep( void* handle, struct gpm_plugin_api_process_attribute_parms* parms );

    /**
    @brief update process model attributes for timestep
    @param handle -  the \a handle
    @param parms -  \a process \a model params
    */
    DLLEXPORT int gpm_plugin_api_update_attributes_timestep( void* handle,
                                                             struct gpm_plugin_api_process_attribute_parms* parms );

#ifdef __cplusplus
}
#endif

#endif
