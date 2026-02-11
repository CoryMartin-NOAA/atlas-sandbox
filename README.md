# Atlas Sandbox

A sandbox repository demonstrating the use of the [ECMWF Atlas](https://github.com/ecmwf/atlas) library for grid interpolation of NetCDF atmospheric data files.

## Overview

This project provides example code that:
- Reads atmospheric data from NetCDF files (e.g., GDAS atmospheric analysis files)
- Uses the ECMWF Atlas library to perform grid-to-grid interpolation
- Supports flexible target grid specifications (Gaussian grids, lat-lon grids, etc.)

## Features

- **NetCDF Reader**: Read atmospheric variables and coordinates from NetCDF files
- **Atlas Interpolation**: Leverage Atlas's powerful interpolation capabilities
- **Flexible Grid Support**: Support for various grid types:
  - Octahedral reduced Gaussian grids (O<n>)
  - Regular Gaussian grids (N<n>)
  - Regular lat-lon grids (L<nx>x<ny>)
- **NetCDF Output**: Write interpolated data to NetCDF files with preserved metadata
- **Batch Processing**: Automatically interpolate all x,y and x,y,z fields in a file

## Prerequisites

This project is designed to work with the JCSDA development environment:
- Docker image: `jcsda/docker-gnu-openmpi-dev:skylab-v8`
- Or a system with the following libraries installed:
  - eckit
  - atlas
  - NetCDF-CXX4

## Building

### Using Dev Container (Recommended)

1. Open the repository in VS Code with the Dev Containers extension
2. VS Code will automatically use the `.devcontainer/devcontainer.json` configuration
3. Build the project:

```bash
mkdir build && cd build
cmake ..
make
```

### Manual Build

If you have the required dependencies installed, ensure the following environment variables are set:

```bash
export netcdf_c_ROOT=/path/to/netcdf-c
export netcdf_cxx4_ROOT=/path/to/netcdf-cxx4
```

Then build:

```bash
mkdir build && cd build
cmake ..
make
```

## Usage

```bash
./build/atlas_interpolate <netcdf_file> <target_grid> <output_file> [variable_name]
```

### Arguments

- `netcdf_file`: Path to the input NetCDF file
- `target_grid`: Target grid specification
  - **Regular lat-lon grids** (recommended for structured output):
    - `L360x181`: Regular lat-lon grid with 360 longitudes and 181 latitudes
    - `L720x361`: Higher resolution regular lat-lon grid
  - **Regular Gaussian grids** (F-type, recommended):
    - `F96`: Regular Gaussian grid with uniform longitude spacing
    - `F48`: Regular Gaussian grid (lower resolution)
  - **Reduced Gaussian grids** (N-type, O-type - may produce unrealistic results with 2D output):
    - `N96`: Reduced Gaussian grid (variable longitude points per latitude)
    - `O32`: Octahedral reduced Gaussian grid
    - ⚠️ **WARNING**: Reduced grids can cause issues with structured 2D output. Use F-type grids instead.
- `output_file`: Path to the output NetCDF file where interpolated data will be written
- `variable_name`: (Optional) Specific variable to interpolate. If not provided, all x,y and x,y,z variables will be interpolated

### Grid Type Recommendations

For best results with structured 2D output (maintaining `time, pfull, grid_yt, grid_xt` dimensions):
- ✅ **Use**: Regular lat-lon grids (`L360x181`) or regular Gaussian grids (`F96`)
- ⚠️ **Avoid**: Reduced Gaussian grids (`N96`, `O32`) - these have variable longitude points per latitude and can produce unrealistic interpolated values when reshaped to 2D

### Examples

Interpolate all variables from a GDAS file to a regular Gaussian grid (F96):

```bash
./build/atlas_interpolate gdas.t00z.atmf006.nc F96 gdas_interpolated.nc
```

Interpolate all variables to a regular lat-lon grid:

```bash
./build/atlas_interpolate gdas.t00z.atmf006.nc L360x181 gdas_interpolated.nc
```

Interpolate only temperature to a 1-degree lat-lon grid:

```bash
./build/atlas_interpolate gdas.t00z.atmf006.nc L360x181 output.nc tmp
```

### Sample Data

Example GDAS atmospheric files can be downloaded from:
```
https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gdas.YYYYMMDD/HH/atmos/
```

For example:
```bash
wget https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gdas.20260211/00/atmos/gdas.t00z.atmf006.nc
```

## Project Structure

```
atlas-sandbox/
├── .devcontainer/
│   └── devcontainer.json    # Dev container configuration
├── src/
│   └── main.cpp             # Main example code
├── CMakeLists.txt           # CMake build configuration
└── README.md                # This file
```

## How It Works

1. **Read NetCDF Data**: The program reads latitude, longitude, and variable data from the input NetCDF file
2. **Create Source Grid**: Constructs an Atlas grid representation of the source data
3. **Create Target Grid**: Creates the target grid based on the user specification
4. **Generate Meshes**: Generates meshes for both source and target grids
5. **Setup Interpolation**: Configures the interpolation scheme (structured linear 2D)
6. **Identify Variables**: Automatically identifies all variables with x,y or x,y,z dimensions (unless a specific variable is requested)
7. **Execute Interpolation**: Performs the interpolation from source to target grid for each variable
8. **Write Output**: Creates a new NetCDF file with interpolated data, preserving metadata and adding interpolation history
9. **Output Results**: Displays statistics and confirms successful interpolation

## References

- [ECMWF Atlas Documentation](https://sites.ecmwf.int/docs/atlas/)
- [Atlas GitHub Repository](https://github.com/ecmwf/atlas)
- [NOAA NOMADS Data Server](https://nomads.ncep.noaa.gov/)
- [JCSDA Docker Images](https://hub.docker.com/r/jcsda/docker-gnu-openmpi-dev)

## License

This project is part of NOAA-EMC Ecosystem. 

See LICENSE and DISCLAIMER for details.

## DISCLAIMER

This repository is a scientific product and is not official communication of the National Oceanic and Atmospheric Administration, or the United States Department of Commerce. All NOAA GitHub project code is provided on an 'as is' basis and the user assumes responsibility for its use. Any claims against the Department of Commerce or Department of Commerce bureaus stemming from the use of this GitHub project will be governed by all applicable Federal law. Any reference to specific commercial products, processes, or services by service mark, trademark, manufacturer, or otherwise, does not constitute or imply their endorsement, recommendation or favoring by the Department of Commerce. The Department of Commerce seal and logo, or the seal and logo of a DOC bureau, shall not be used in any manner to imply endorsement of any commercial product or activity by DOC or the United States Government.
