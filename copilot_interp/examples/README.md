# Examples

This directory contains example scripts and usage demonstrations for the atlas-sandbox project.

## Quick Start

### 1. Build the Project

First, build the project using the provided build script:

```bash
./build.sh
```

Or manually:

```bash
mkdir build && cd build
cmake ..
make
cd ..
```

### 2. Run the Example Script

The `run_example.py` script will download a sample GDAS file and demonstrate interpolation:

```bash
python3 examples/run_example.py
```

This script will:
- Download a recent GDAS atmospheric file
- Demonstrate interpolation to an O32 octahedral Gaussian grid
- Demonstrate interpolation to a 1-degree lat-lon grid (L360x181)

### 3. Manual Usage

You can also run the interpolation directly:

```bash
# Download a sample file (or use your own)
wget https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gdas.20260211/00/atmos/gdas.t00z.atmf006.nc

# Run interpolation to O32 grid
./build/atlas_interpolate gdas.t00z.atmf006.nc O32 tmp

# Run interpolation to a custom lat-lon grid
./build/atlas_interpolate gdas.t00z.atmf006.nc L720x361 tmp
```

## Grid Specifications

The Atlas library supports various grid types:

### Octahedral Reduced Gaussian Grids
- Format: `O<n>` where n is the number of latitude lines
- Examples: `O32`, `O48`, `O96`, `O128`
- These are efficient grids used by many weather models

### Regular Gaussian Grids
- Format: `N<n>` where n is the number of latitude lines between pole and equator
- Examples: `N32`, `N64`, `N128`
- Traditional Gaussian grids with regular longitude spacing

### Regular Lat-Lon Grids
- Format: `L<nx>x<ny>` where nx = longitudes, ny = latitudes
- Examples:
  - `L360x181` (1-degree grid: 360 longitudes × 181 latitudes)
  - `L720x361` (0.5-degree grid: 720 longitudes × 361 latitudes)
  - `L1440x721` (0.25-degree grid: 1440 longitudes × 721 latitudes)

## Common Variables in GDAS Files

The GDAS atmospheric files contain many variables. Common ones include:

- `tmp` - Temperature
- `spfh` - Specific humidity
- `ugrd` - U-component of wind
- `vgrd` - V-component of wind
- `pressfc` - Surface pressure
- `hgtsfc` - Surface geopotential height

You can list all variables by running the program with any variable name and checking the output.

## Troubleshooting

### File Not Found
If the download fails, the file may not be available yet or has been archived. Try:
- Using a date 1-2 days in the past
- Checking the NOMADS website: https://nomads.ncep.noaa.gov/
- Using a different cycle time (00, 06, 12, or 18Z)

### Build Errors
Make sure you're using the devcontainer environment which has all required dependencies:
- eckit
- atlas
- NetCDF-CXX4

### Variable Not Found
Use the program's info output to see available variables in your NetCDF file.
