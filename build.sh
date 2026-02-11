#!/bin/bash

# Build script for atlas-sandbox

set -e  # Exit on error

echo "=== Atlas Sandbox Build Script ==="
echo ""

# Check for required dependencies
echo "Checking for required tools..."
command -v cmake >/dev/null 2>&1 || { echo "ERROR: cmake is required but not installed."; exit 1; }
command -v g++ >/dev/null 2>&1 || { echo "ERROR: g++ is required but not installed."; exit 1; }

echo "✓ cmake found: $(cmake --version | head -n1)"
echo "✓ g++ found: $(g++ --version | head -n1)"
echo ""

# Create build directory
echo "Creating build directory..."
rm -rf build
mkdir -p build
cd build

# Run CMake
echo ""
echo "Running CMake configuration..."
if cmake ..; then
    echo "✓ CMake configuration successful"
else
    echo "✗ CMake configuration failed"
    echo ""
    echo "This is expected if eckit, atlas, or NetCDF-CXX are not installed."
    echo "Please use the devcontainer environment to build this project."
    exit 1
fi

# Build
echo ""
echo "Building project..."
if make; then
    echo ""
    echo "✓ Build successful!"
    echo ""
    echo "Executable: build/atlas_interpolate"
else
    echo "✗ Build failed"
    exit 1
fi

echo ""
echo "=== Build Complete ==="
