cmake_minimum_required(VERSION 3.12)
project(atlas-sandbox VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(eckit REQUIRED)
find_package(atlas REQUIRED)

# Handle NetCDF with support for netcdf_c_ROOT and netcdf_cxx4_ROOT environment variables
# These are commonly used in JCSDA environments
# Append to CMAKE_PREFIX_PATH to help find_package locate NetCDF
if(DEFINED ENV{netcdf_c_ROOT})
    list(APPEND CMAKE_PREFIX_PATH $ENV{netcdf_c_ROOT})
endif()
if(DEFINED ENV{netcdf_cxx4_ROOT})
    list(APPEND CMAKE_PREFIX_PATH $ENV{netcdf_cxx4_ROOT})
endif()

# Try to find NetCDF using config mode first
find_package(NetCDF QUIET CONFIG COMPONENTS CXX)

# If not found, try with module mode or manual detection
if(NOT NetCDF_FOUND)
    # Try to find netcdf-cxx4 directly (note: different casing used by different distributions)
    find_package(netCDF QUIET CONFIG)
    if(netCDF_FOUND)
        set(NetCDF_FOUND TRUE)
        # Create imported target if it doesn't exist
        if(NOT TARGET NetCDF::NetCDF_CXX AND TARGET netCDF::netcdf)
            add_library(NetCDF::NetCDF_CXX INTERFACE IMPORTED)
            target_link_libraries(NetCDF::NetCDF_CXX INTERFACE netCDF::netcdf)
        endif()
    else()
        # Manual detection using environment variables
        if(DEFINED ENV{netcdf_cxx4_ROOT})
            # Try to find the include directory with header validation
            find_path(NETCDF_CXX_INCLUDE_DIR
                NAMES ncVar.h
                HINTS $ENV{netcdf_cxx4_ROOT}
                PATH_SUFFIXES include include/netcdf
                NO_DEFAULT_PATH
            )
            
            # Try to find the library with platform-independent search
            find_library(NETCDF_CXX_LIBRARY
                NAMES netcdf-cxx4 netcdf_c++4
                HINTS $ENV{netcdf_cxx4_ROOT}
                PATH_SUFFIXES lib lib64 lib/x86_64-linux-gnu Library/lib bin
                NO_DEFAULT_PATH
            )
            
            if(NETCDF_CXX_LIBRARY AND NETCDF_CXX_INCLUDE_DIR)
                set(NetCDF_FOUND TRUE)
                add_library(NetCDF::NetCDF_CXX INTERFACE IMPORTED)
                target_include_directories(NetCDF::NetCDF_CXX INTERFACE ${NETCDF_CXX_INCLUDE_DIR})
                target_link_libraries(NetCDF::NetCDF_CXX INTERFACE ${NETCDF_CXX_LIBRARY})
                message(STATUS "Found NetCDF C++ using netcdf_cxx4_ROOT: ${NETCDF_CXX_LIBRARY}")
            endif()
        endif()
    endif()
endif()

if(NOT NetCDF_FOUND)
    message(FATAL_ERROR "NetCDF C++ library not found. Please ensure:\n"
                        "  1. NetCDF-CXX4 is installed, or\n"
                        "  2. Set netcdf_cxx4_ROOT environment variable pointing to NetCDF-CXX4 installation\n"
                        "     (netcdf_c_ROOT can also help CMake find NetCDF C library)")
endif()

# Create executable
add_executable(atlas_interpolate
    src/main.cpp
)

# Link libraries
target_link_libraries(atlas_interpolate
    PRIVATE
        atlas
        eckit
        NetCDF::NetCDF_CXX
)

# Include directories
target_include_directories(atlas_interpolate
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Installation
install(TARGETS atlas_interpolate
    RUNTIME DESTINATION bin
)

# Print configuration summary
message(STATUS "")
message(STATUS "atlas-sandbox configuration:")
message(STATUS "  C++ compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "  C++ standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Install prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "")
