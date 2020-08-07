cmake_minimum_required(VERSION 3.1)

project(gpm_plugin_description)

add_library(gpm_plugin_description INTERFACE)
target_sources(gpm_plugin_description INTERFACE
${CMAKE_CURRENT_SOURCE_DIR}/gpm_plugin_description.h
${CMAKE_CURRENT_SOURCE_DIR}/gpm_plugin_helpers.h
${CMAKE_CURRENT_SOURCE_DIR}/gpm_plugin_lin_span.h
${CMAKE_CURRENT_SOURCE_DIR}/gpm_plugin_lin_multi_span.h
)
target_include_directories(gpm_plugin_description 
INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)


