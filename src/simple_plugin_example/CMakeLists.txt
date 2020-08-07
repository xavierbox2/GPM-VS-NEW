cmake_minimum_required(VERSION 3.1)

project ( simple_plugin_process)


add_library(simple_plugin_process SHARED "" )
target_sources(simple_plugin_process
PRIVATE
simple_plugin_process_api.cxx
simple_plugin_process_api.h
simple_plugin_process.cxx
simple_plugin_process.h
)
target_link_libraries(simple_plugin_process 
PUBLIC gpm_plugin_description )

target_include_directories(simple_plugin_process 
  PUBLIC
     $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_property(TARGET simple_plugin_process PROPERTY POSITION_INDEPENDENT_CODE ON)
