cmake_minimum_required(VERSION 3.13)

project(akaifat)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-fPIC")

if(DEFINED ENV{APPLE_M1})
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE INTERNAL "")
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9" CACHE INTERNAL "Minimum OS X deployment version")
endif()

if(EXISTS ${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo_multi.cmake)
else()
    include(${CMAKE_CURRENT_BINARY_DIR}/conanbuildinfo.cmake)
endif()

conan_basic_setup(NO_OUTPUT_DIRS)

## Configure main lib ##

set(_src_root_path_main "${CMAKE_CURRENT_SOURCE_DIR}/main")

include_directories(${_src_root_path_main})

file(
    GLOB_RECURSE _source_list_main 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_main}/*.c"
    "${_src_root_path_main}/*.m"
    "${_src_root_path_main}/*.mm"
    "${_src_root_path_main}/*.cpp"
    "${_src_root_path_main}/*.h"
    "${_src_root_path_main}/*.hpp"
	"${_src_root_path_main}/*.hxx"
)

foreach(_source IN ITEMS ${_source_list_main})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

add_library(akaifat ${_source_list_main})
conan_target_link_libraries(akaifat)

## Configure test suite application ##

set(_src_root_path_test "${CMAKE_CURRENT_SOURCE_DIR}/test")

include_directories(${_src_root_path_test})

file(
    GLOB_RECURSE _source_list_test 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_test}/*.c*"
    "${_src_root_path_test}/*.cpp*"
    "${_src_root_path_test}/*.hpp*"
)

foreach(_source IN ITEMS ${_source_list_test})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

add_executable(akaifat-tests ${_source_list_test})
target_link_libraries(akaifat-tests akaifat)


# Configure Windows raw access service

set(_src_root_path_raw_access_svc "${CMAKE_CURRENT_SOURCE_DIR}/win_raw_access_service")

file(
    GLOB_RECURSE _source_list_win_raw_access_svc 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_raw_access_svc}/RawAccessSvc.cpp"
)

add_executable(raw-access-service ${_source_list_win_raw_access_svc})

file(
    GLOB_RECURSE _source_list_win_raw_access_svc_config 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_raw_access_svc}/RawAccessSvcConfig.cpp"
)

add_executable(raw-access-service-config ${_source_list_win_raw_access_svc_config})

file(
    GLOB_RECURSE _source_list_win_raw_access_svc_control 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_raw_access_svc}/RawAccessSvcControl.cpp"
)

add_executable(raw-access-service-control ${_source_list_win_raw_access_svc_control})
