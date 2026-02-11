# Contributing to atlas-sandbox

Thank you for your interest in contributing to atlas-sandbox!

## Development Environment

### Using DevContainer (Recommended)

The easiest way to develop is using the provided devcontainer:

1. Open the repository in VS Code
2. Install the "Dev Containers" extension
3. When prompted, click "Reopen in Container"
4. The container will automatically set up the environment with all dependencies

The devcontainer uses the `jcsda/docker-gnu-openmpi-dev:skylab-v8` image which includes:
- eckit
- atlas
- NetCDF-CXX4
- CMake, GCC, and other build tools

### Manual Setup

If you prefer not to use devcontainer, ensure you have these dependencies installed:
- eckit (ECMWF toolkit)
- atlas (ECMWF Atlas library)
- NetCDF-CXX4 (NetCDF C++ bindings)
- CMake >= 3.12
- C++17 compatible compiler

Set the following environment variables to help CMake find NetCDF:
```bash
export netcdf_c_ROOT=/path/to/netcdf-c
export netcdf_cxx4_ROOT=/path/to/netcdf-cxx4
```

## Building the Project

```bash
# Create build directory
mkdir build
cd build

# Configure with CMake
cmake ..

# Build
make

# The executable will be at: build/atlas_interpolate
```

Or use the provided build script:

```bash
./build.sh
```

## Code Style

- Use C++17 features where appropriate
- Follow existing code formatting and naming conventions
- Add comments for complex logic
- Keep functions focused and modular

## Testing Changes

1. Build the project after making changes
2. Test with example data if possible:
   ```bash
   # Download sample data
   wget https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/gdas.YYYYMMDD/HH/atmos/gdas.tHHz.atmf006.nc
   
   # Test your changes
   ./build/atlas_interpolate gdas.tHHz.atmf006.nc O32 tmp
   ```
3. Verify the output is reasonable

## Adding New Features

When adding new features:
1. Keep changes minimal and focused
2. Update documentation (README, examples)
3. Follow the existing code structure
4. Test with real data when possible

## Common Development Tasks

### Adding a New Grid Type
- Modify the grid creation logic in `AtlasInterpolator::createSourceGrid()`
- Update documentation with new grid specifications

### Supporting New NetCDF Formats
- Extend `NetCDFReader` to handle different variable naming conventions
- Add logic to detect and handle different dimension orderings

### Adding New Interpolation Methods
- Atlas supports various interpolation schemes
- Modify the interpolation setup in `AtlasInterpolator` constructor
- Document the new method in README

## Questions or Issues?

If you have questions or encounter issues:
1. Check the README and examples/README.md
2. Review existing code and comments
3. Open an issue on GitHub

## License

By contributing, you agree that your contributions will be licensed under the same license as the project (see LICENSE file).
