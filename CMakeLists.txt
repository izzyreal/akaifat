cmake_minimum_required(VERSION 3.13)

include(FetchContent)

set(CATCH_CONFIG_NO_CPP17_UNCAUGHT_EXCEPTIONS ON)

FetchContent_Declare(Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.3.2
  SOURCE_DIR     ${CMAKE_SOURCE_DIR}/deps/Catch2
)

FetchContent_MakeAvailable(Catch2)

project(akaifat)

set(CMAKE_CXX_STANDARD 17)

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "10.11" CACHE INTERNAL "Minimum OS X deployment version")
endif()

## Configure main lib ##

set(_src_root_path_main "${CMAKE_CURRENT_SOURCE_DIR}/src/main")

include_directories(${_src_root_path_main})

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    set(MACOSX TRUE)
endif()

if (MACOSX)
    file(
        GLOB_RECURSE _source_list_main 
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_src_root_path_main}/*.c*"
        "${_src_root_path_main}/*.h*"
        "${_src_root_path_main}/*.m*"
    )
else()
    file(
        GLOB_RECURSE _source_list_main 
        LIST_DIRECTORIES false
        RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
        "${_src_root_path_main}/*.c*"
        "${_src_root_path_main}/*.h*"
    )    
endif()

foreach(_source IN ITEMS ${_source_list_main})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()


add_library(akaifat ${_source_list_main})

if (UNIX AND NOT APPLE)
    include(FindPkgConfig)
    pkg_search_module(udisks2 REQUIRED udisks2)
    target_include_directories(akaifat SYSTEM PUBLIC ${udisks2_INCLUDE_DIRS})
    target_link_libraries(akaifat ${udisks2_LIBRARIES})
endif()

if (APPLE AND NOT CMAKE_SYSTEM_NAME STREQUAL iOS)
    target_link_libraries(akaifat stdc++ "-framework Foundation -framework Security -framework DiskArbitration -framework SystemConfiguration")
endif()

## Configure test suite application ##

set(_src_root_path_test "${CMAKE_CURRENT_SOURCE_DIR}/src/test")

include_directories(${_src_root_path_test})

file(
    GLOB_RECURSE _source_list_test 
    LIST_DIRECTORIES false
    RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
    "${_src_root_path_test}/*.c*"
    "${_src_root_path_test}/*.h*"
)

foreach(_source IN ITEMS ${_source_list_test})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
    source_group("${_source_path_msvc}" FILES "${_source}")
endforeach()

add_executable(akaifat-tests ${_source_list_test} src/test/FatTests.cpp)

if (UNIX AND NOT APPLE)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    target_link_libraries(akaifat-tests Threads::Threads ${udisks2_LIBRARIES} akaifat Catch2::Catch2WithMain)
else()
    target_link_libraries(akaifat-tests akaifat Catch2::Catch2WithMain)
endif()
