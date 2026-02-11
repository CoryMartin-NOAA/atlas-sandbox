cmake_minimum_required(VERSION 3.12)
project(atlas-sandbox VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(eckit REQUIRED)
find_package(atlas REQUIRED)
find_package(NetCDF REQUIRED COMPONENTS CXX)

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
