#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <set>
#include <algorithm>
#include <limits>
#include <netcdf>
#include "atlas/grid.h"
#include "atlas/grid/StructuredGrid.h"
#include "atlas/mesh.h"
#include "atlas/meshgenerator.h"
#include "atlas/functionspace.h"
#include "atlas/field.h"
#include "atlas/array.h"
#include "atlas/interpolation.h"
#include "atlas/option.h"
#include "atlas/library/Library.h"
#include "eckit/log/Log.h"

class NetCDFReader {
public:
    NetCDFReader(const std::string& filename) : filename_(filename) {
        try {
            ncfile_ = std::make_unique<netCDF::NcFile>(filename, netCDF::NcFile::read);
            std::cout << "Successfully opened: " << filename << std::endl;
        } catch (netCDF::exceptions::NcException& e) {
            std::cerr << "Error opening NetCDF file: " << e.what() << std::endl;
            throw;
        }
    }

    void printInfo() {
        std::cout << "\n=== NetCDF File Information ===" << std::endl;
        std::cout << "File: " << filename_ << std::endl;
        
        // Print dimensions
        std::cout << "\nDimensions:" << std::endl;
        auto dims = ncfile_->getDims();
        for (const auto& dim : dims) {
            std::cout << "  " << dim.first << ": " << dim.second.getSize() << std::endl;
        }
        
        // Print variables
        std::cout << "\nVariables:" << std::endl;
        auto vars = ncfile_->getVars();
        for (const auto& var : vars) {
            std::cout << "  " << var.first << " (";
            auto dims = var.second.getDims();
            for (size_t i = 0; i < dims.size(); i++) {
                if (i > 0) std::cout << ", ";
                std::cout << dims[i].getName();
            }
            std::cout << ")" << std::endl;
        }
    }

    bool hasVariable(const std::string& varName) {
        auto vars = ncfile_->getVars();
        return vars.find(varName) != vars.end();
    }

    std::vector<double> readLatitudes() {
        // Try common latitude variable names
        // First try 1D coordinate variables (for regular lat-lon grids)
        std::vector<std::string> latNames = {"grid_yt", "latitude", "lat"};
        
        for (const auto& name : latNames) {
            if (hasVariable(name)) {
                netCDF::NcVar latVar = ncfile_->getVar(name);
                auto dims = latVar.getDims();
                
                // Only read 1D latitude coordinates
                if (dims.size() == 1) {
                    std::vector<double> lats(dims[0].getSize());
                    latVar.getVar(lats.data());
                    std::cout << "Read " << lats.size() << " latitude values from '" << name << "'" << std::endl;
                    return lats;
                } else {
                    std::cerr << "Warning: '" << name << "' is " << dims.size() << "D (expected 1D)" << std::endl;
                }
            }
        }
        
        std::cerr << "Error: Could not find 1D latitude coordinate variable." << std::endl;
        std::cerr << "This tool currently only supports regular lat-lon grids." << std::endl;
        std::cerr << "Curvilinear grids (with 2D lat/lon arrays) are not yet supported." << std::endl;
        return {};
    }

    std::vector<double> readLongitudes() {
        // Try common longitude variable names
        // First try 1D coordinate variables (for regular lat-lon grids)
        std::vector<std::string> lonNames = {"grid_xt", "longitude", "lon"};
        
        for (const auto& name : lonNames) {
            if (hasVariable(name)) {
                netCDF::NcVar lonVar = ncfile_->getVar(name);
                auto dims = lonVar.getDims();
                
                // Only read 1D longitude coordinates
                if (dims.size() == 1) {
                    std::vector<double> lons(dims[0].getSize());
                    lonVar.getVar(lons.data());
                    std::cout << "Read " << lons.size() << " longitude values from '" << name << "'" << std::endl;
                    return lons;
                } else {
                    std::cerr << "Warning: '" << name << "' is " << dims.size() << "D (expected 1D)" << std::endl;
                }
            }
        }
        
        std::cerr << "Error: Could not find 1D longitude coordinate variable." << std::endl;
        std::cerr << "This tool currently only supports regular lat-lon grids." << std::endl;
        std::cerr << "Curvilinear grids (with 2D lat/lon arrays) are not yet supported." << std::endl;
        return {};
    }

    std::vector<float> readVariable(const std::string& varName) {
        if (!hasVariable(varName)) {
            std::cerr << "Variable '" << varName << "' not found" << std::endl;
            return {};
        }

        netCDF::NcVar var = ncfile_->getVar(varName);
        auto dims = var.getDims();
        
        std::cout << "Reading variable: " << varName << " (";
        size_t totalSize = 1;
        for (size_t i = 0; i < dims.size(); i++) {
            if (i > 0) std::cout << " x ";
            std::cout << dims[i].getSize();
            totalSize *= dims[i].getSize();
        }
        std::cout << ")" << std::endl;

        std::vector<float> data(totalSize);
        var.getVar(data.data());
        
        return data;
    }

    netCDF::NcVar getVariable(const std::string& varName) {
        return ncfile_->getVar(varName);
    }

    std::multimap<std::string, netCDF::NcVar> getAllVariables() {
        return ncfile_->getVars();
    }

    std::multimap<std::string, netCDF::NcDim> getAllDimensions() {
        return ncfile_->getDims();
    }

    std::multimap<std::string, netCDF::NcGroupAtt> getGlobalAttributes() {
        return ncfile_->getAtts();
    }

    std::string getLatitudeDimName() {
        std::vector<std::string> latNames = {"lat", "latitude", "grid_yt", "yt"};
        auto dims = ncfile_->getDims();
        for (const auto& name : latNames) {
            if (dims.find(name) != dims.end()) {
                return name;
            }
        }
        return "";
    }

    std::string getLongitudeDimName() {
        std::vector<std::string> lonNames = {"lon", "longitude", "grid_xt", "xt"};
        auto dims = ncfile_->getDims();
        for (const auto& name : lonNames) {
            if (dims.find(name) != dims.end()) {
                return name;
            }
        }
        return "";
    }

private:
    std::string filename_;
    std::unique_ptr<netCDF::NcFile> ncfile_;
};

class NetCDFWriter {
public:
    NetCDFWriter(const std::string& filename) : filename_(filename) {
        try {
            ncfile_ = std::make_unique<netCDF::NcFile>(filename, netCDF::NcFile::replace);
            std::cout << "Created output file: " << filename << std::endl;
        } catch (netCDF::exceptions::NcException& e) {
            std::cerr << "Error creating NetCDF file: " << e.what() << std::endl;
            throw;
        }
    }

    void copyGlobalAttributes(const std::multimap<std::string, netCDF::NcGroupAtt>& attrs) {
        std::cout << "Copying global attributes..." << std::endl;
        for (const auto& attr : attrs) {
            copyAttribute(attr.second, ncfile_.get());
        }
    }

    void addInterpolationHistory(const std::string& sourceFile, 
                                  const std::string& originalResolution,
                                  const std::string& targetGrid) {
        std::string history = "Interpolated from " + sourceFile + 
                              " (original resolution: " + originalResolution + 
                              ") to " + targetGrid + " using Atlas interpolation";
        ncfile_->putAtt("interpolation_history", history);
        std::cout << "Added interpolation history attribute" << std::endl;
    }

    netCDF::NcDim addDimension(const std::string& name, size_t size) {
        return ncfile_->addDim(name, size);
    }

    netCDF::NcVar addVariable(const std::string& name, netCDF::NcType type,
                              const std::vector<netCDF::NcDim>& dims) {
        return ncfile_->addVar(name, type, dims);
    }

    void writeVariableData(const std::string& varName, const std::vector<double>& data) {
        auto var = ncfile_->getVar(varName);
        if (var.isNull()) {
            std::cerr << "Variable " << varName << " not found for writing" << std::endl;
            return;
        }
        var.putVar(data.data());
    }

    void writeVariableData(const std::string& varName, const std::vector<float>& data) {
        auto var = ncfile_->getVar(varName);
        if (var.isNull()) {
            std::cerr << "Variable " << varName << " not found for writing" << std::endl;
            return;
        }
        var.putVar(data.data());
    }

    void copyVariableAttributes(const netCDF::NcVar& sourceVar, const std::string& targetVarName) {
        auto targetVar = ncfile_->getVar(targetVarName);
        if (targetVar.isNull()) {
            std::cerr << "Target variable " << targetVarName << " not found" << std::endl;
            return;
        }
        
        auto attrs = sourceVar.getAtts();
        for (const auto& attr : attrs) {
            copyAttribute(attr.second, &targetVar);
        }
    }

private:
    std::string filename_;
    std::unique_ptr<netCDF::NcFile> ncfile_;

    void copyAttribute(const netCDF::NcAtt& attr, netCDF::NcGroup* group) {
        auto type = attr.getType();
        size_t len = attr.getAttLength();
        
        if (type == netCDF::ncChar) {
            std::string value;
            attr.getValues(value);
            group->putAtt(attr.getName(), value);
        } else if (type == netCDF::ncFloat) {
            std::vector<float> values(len);
            attr.getValues(values.data());
            group->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncDouble) {
            std::vector<double> values(len);
            attr.getValues(values.data());
            group->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncInt) {
            std::vector<int> values(len);
            attr.getValues(values.data());
            group->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncShort) {
            std::vector<short> values(len);
            attr.getValues(values.data());
            group->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncByte) {
            std::vector<signed char> values(len);
            attr.getValues(values.data());
            group->putAtt(attr.getName(), type, len, values.data());
        }
    }

    void copyAttribute(const netCDF::NcAtt& attr, netCDF::NcVar* var) {
        auto type = attr.getType();
        size_t len = attr.getAttLength();
        
        if (type == netCDF::ncChar) {
            std::string value;
            attr.getValues(value);
            var->putAtt(attr.getName(), value);
        } else if (type == netCDF::ncFloat) {
            std::vector<float> values(len);
            attr.getValues(values.data());
            var->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncDouble) {
            std::vector<double> values(len);
            attr.getValues(values.data());
            var->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncInt) {
            std::vector<int> values(len);
            attr.getValues(values.data());
            var->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncShort) {
            std::vector<short> values(len);
            attr.getValues(values.data());
            var->putAtt(attr.getName(), type, len, values.data());
        } else if (type == netCDF::ncByte) {
            std::vector<signed char> values(len);
            attr.getValues(values.data());
            var->putAtt(attr.getName(), type, len, values.data());
        }
    }
};

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <netcdf_file> <target_grid> <output_file>" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  netcdf_file    : Path to NetCDF file to read" << std::endl;
    std::cout << "  target_grid    : Target grid specification (e.g., 'O32', 'F32', 'L360x181')" << std::endl;
    std::cout << "  output_file    : Path to output NetCDF file" << std::endl;
    std::cout << "\nGrid specifications:" << std::endl;
    std::cout << "  O<n>     : Octahedral reduced Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  N<n>     : Regular Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  L<nx>x<ny> : Regular lat-lon grid with <nx> longitudes and <ny> latitudes" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << progName << " gdas.t00z.atmf006.nc F96 gdas_interpolated.nc" << std::endl;
    std::cout << "  " << progName << " gdas.t00z.atmf006.nc L360x181 output.nc" << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize Atlas library
    atlas::Library::instance().initialise(argc, argv);
    
    std::cout << "=== Atlas NetCDF Calculation and Interpolation Example ===" << std::endl;
    
    // Parse command line arguments
    if (argc < 4) {
        printUsage(argv[0]);
        atlas::Library::instance().finalise();
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string targetGridSpec = argv[2];
    std::string outputFile = argv[3];
    // Open NetCDF file and read data (not shown here, but you would use NetCDF C++ API to read the data into vectors)
    // Read NetCDF file
    NetCDFReader reader(inputFile);
    reader.printInfo();
    
    // Read coordinates
    auto lats = reader.readLatitudes();
    auto lons = reader.readLongitudes();
    
    if (lats.empty() || lons.empty()) {
        std::cerr << "Error: Could not read coordinates from NetCDF file" << std::endl;
        atlas::Library::instance().finalise();
        return 1;
    }
    
    // Check latitude ordering in input file
    std::cout << "Input NetCDF latitude ordering check:" << std::endl;
    std::cout << "  First few latitudes: ";
    for (size_t i = 0; i < std::min(size_t(5), lats.size()); ++i) {
        std::cout << lats[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "  Last few latitudes: ";
    size_t start = std::max(size_t(0), lats.size() - 5);
    for (size_t i = start; i < lats.size(); ++i) {
        std::cout << lats[i] << " ";
    }
    std::cout << std::endl;
    
    // Check if latitudes are in descending order (north to south)
    bool isNorthToSouth = lats.size() > 1 && lats[0] > lats[lats.size()-1];
    std::cout << "  Latitude ordering: " << (isNorthToSouth ? "North-to-South" : "South-to-North") << std::endl;
    
    // Atlas typically expects latitudes in ascending order (south to north)
    // If NetCDF has north-to-south, we need to flip the latitude indexing
    bool needLatFlip = isNorthToSouth;
    if (needLatFlip) {
        std::cout << "  WARNING: NetCDF latitudes are North-to-South, but Atlas expects South-to-North" << std::endl;
        std::cout << "  Will apply latitude flipping when copying data to Atlas field" << std::endl;
    }
    
    std::string latDimName = reader.getLatitudeDimName();
    std::string lonDimName = reader.getLongitudeDimName();
    
    // Read temperature
    auto tempIn = reader.getVariable("tmp");
    std::cout << "Read variable 'tmp' with dimensions: ";
    auto tempDims = tempIn.getDims();
    for (size_t i = 0; i < tempDims.size(); i++) {
        if (i > 0) std::cout << " x ";
        std::cout << tempDims[i].getName() << "(" << tempDims[i].getSize() << ")";
    }
    std::cout << std::endl;

    // create an atlas functionspace
    std::string atlasInputGridName;
    // Create a regular Gaussian grid specification based on actual grid dimensions
    atlasInputGridName = "F" + std::to_string(lats.size() / 2);
    std::cout << "Using input grid specification: " << atlasInputGridName << std::endl;
    const atlas::Grid gridInput(atlasInputGridName);
    eckit::LocalConfiguration atlas_conf;
    atlas_conf.set("halo", 1); // Add halo for structured interpolation
    atlas::functionspace::StructuredColumns inputFunctionSpace_(gridInput, atlas_conf);

    // Create atlas fieldset from the input data
    atlas::FieldSet inFs;

    // Read temperature data from NetCDF variable
    size_t totalTempSize = 1;
    std::vector<size_t> tempShape;
    for (const auto& dim : tempDims) {
        size_t dimSize = dim.getSize();
        tempShape.push_back(dimSize);
        totalTempSize *= dimSize;
        std::cout << "Temperature dimension " << dim.getName() << ": " << dimSize << std::endl;
    }
    
    // Read the actual temperature data
    std::vector<float> tempData(totalTempSize);
    tempIn.getVar(tempData.data());
    std::cout << "Successfully read " << totalTempSize << " temperature values" << std::endl;
    
    // Print min/max values from input NetCDF data
    auto minIt = std::min_element(tempData.begin(), tempData.end());
    auto maxIt = std::max_element(tempData.begin(), tempData.end());
    std::cout << "Input NetCDF temperature range: [" << *minIt << ", " << *maxIt << "]" << std::endl;
    
    // Declare variables that will be used in multiple sections
    size_t numLevels = 1;
    size_t spatialSize = inputFunctionSpace_.size(); // number of grid points
    
    // Determine the number of vertical levels by looking at dimension names
    for (const auto& dim : tempDims) {
        std::string dimName = dim.getName();
        // Common level dimension names in atmospheric data
        if (dimName == "pfull" || dimName == "lev" || dimName == "level" || 
            dimName == "plev" || dimName == "pressure" || dimName == "z" ||
            dimName == "height" || dimName == "vertical") {
            numLevels = dim.getSize();
            std::cout << "Found vertical dimension '" << dimName << "' with " 
                      << numLevels << " levels" << std::endl;
            break;
        }
    }
    
    if (numLevels == 1 && tempShape.size() >= 3) {
        std::cout << "Warning: Could not identify level dimension by name, using dimension order assumption" << std::endl;
        // Fallback: assume first dimension is levels if we have 3+ dimensions
        numLevels = tempShape[0];
        std::cout << "Assuming first dimension is levels: " << numLevels << std::endl;
    }
    
    // Create Atlas field for temperature
    // Atlas fields typically expect [nodes] for 2D or [nodes, levels] for 3D
    if (tempShape.size() >= 2) {
        std::cout << "Creating Atlas field with " << numLevels << " vertical levels" << std::endl;
        // Create the temperature field
        atlas::Field tempField;
        if (numLevels > 1) {
            // 3D field with vertical levels
            tempField = inputFunctionSpace_.createField<float>(
                atlas::option::name("temperature") | 
                atlas::option::levels(numLevels)
            );
        } else {
            // 2D field
            tempField = inputFunctionSpace_.createField<float>(
                atlas::option::name("temperature")
            );
        }
        std::cout << "Created Atlas field for temperature with shape: ";
        for (int i = 0; i < tempField.rank(); ++i) {
            if (i > 0) std::cout << " x ";
            std::cout << tempField.shape(i);
        }
        std::cout << std::endl;
        
        // Copy data to Atlas field using appropriate view based on dimensions
        std::cout << "Copying temperature data to Atlas field..." << std::endl;
        
        if (numLevels > 1) {
            // 3D field: use 2D view (nodes x levels)
            auto tempView = atlas::array::make_view<float, 2>(tempField);
            
            // Copy data from NetCDF format to Atlas field format
            // NetCDF data is in [time, level, lat, lon] order - we need to skip time dimension (assume time=0)
            size_t timeIndex = 0; // Use first (and only) time step
            size_t latSize = 0, lonSize = 0;
            
            // Find lat and lon dimensions
            for (const auto& dim : tempDims) {
                if (dim.getName() == latDimName) {
                    latSize = dim.getSize();
                } else if (dim.getName() == lonDimName) {
                    lonSize = dim.getSize();
                }
            }
            
            // Copy data accounting for [time, level, lat, lon] ordering and latitude flipping
            for (size_t level = 0; level < numLevels; ++level) {
                for (size_t lat = 0; lat < latSize; ++lat) {
                    for (size_t lon = 0; lon < lonSize; ++lon) {
                        size_t netcdfIndex = timeIndex * (numLevels * latSize * lonSize) + 
                                           level * (latSize * lonSize) + 
                                           lat * lonSize + 
                                           lon;
                        
                        // Apply latitude flipping if needed (NetCDF N->S to Atlas S->N)
                        size_t atlasLat = needLatFlip ? (latSize - 1 - lat) : lat;
                        size_t atlasIndex = atlasLat * lonSize + lon; // atlas spatial index
                        
                        if (netcdfIndex < tempData.size() && atlasIndex < spatialSize) {
                            tempView(atlasIndex, level) = tempData[netcdfIndex];
                        }
                    }
                }
            }
        } else {
            // 2D field: use 1D view (just nodes)
            auto tempView = atlas::array::make_view<float, 1>(tempField);
            
            // For 2D data, we need to find lat and lon dimensions to handle flipping
            size_t latSize = 0, lonSize = 0;
            for (const auto& dim : tempDims) {
                if (dim.getName() == latDimName) {
                    latSize = dim.getSize();
                } else if (dim.getName() == lonDimName) {
                    lonSize = dim.getSize();
                }
            }
            
            if (latSize > 0 && lonSize > 0 && latSize * lonSize == tempData.size()) {
                // Data is structured as [lat, lon] - apply latitude flipping
                std::cout << "2D data structure: lat(" << latSize << ") x lon(" << lonSize << ")" << std::endl;
                for (size_t lat = 0; lat < latSize; ++lat) {
                    for (size_t lon = 0; lon < lonSize; ++lon) {
                        size_t netcdfIndex = lat * lonSize + lon;
                        
                        // Apply latitude flipping if needed
                        size_t atlasLat = needLatFlip ? (latSize - 1 - lat) : lat;
                        size_t atlasIndex = atlasLat * lonSize + lon;
                        
                        if (netcdfIndex < tempData.size() && atlasIndex < spatialSize) {
                            tempView(atlasIndex) = tempData[netcdfIndex];
                        }
                    }
                }
            } else {
                // Fallback: copy data as-is (may not be structured in lat-lon order)
                std::cout << "Warning: Cannot determine 2D data structure, copying as-is" << std::endl;
                for (size_t i = 0; i < spatialSize && i < tempData.size(); ++i) {
                    tempView(i) = tempData[i];
                }
            }
        }
        
        std::cout << "Finished copying temperature data to Atlas field" << std::endl;
        // Add field to fieldset
        inFs.add(tempField);
        std::cout << "Added temperature field to fieldset with " << numLevels << " levels" << std::endl;
        
    } else {
        std::cerr << "Error: Temperature field has insufficient dimensions" << std::endl;
    }

    // Calculate and add a new field (e.g., temperature in Celsius from Kelvin)
    
    // Create a new field for temperature in Fahrenheit from the existing temperature field
    atlas::Field tempField = inFs["temperature"];
    if (!tempField.valid()) {
        std::cerr << "Error: Temperature field not found in fieldset" << std::endl;
    } else {
        std::cout << "Creating temperature in Fahrenheit field..." << std::endl;
        
        // Create new field with same structure as temperature field
        atlas::Field tempFField;
        if (numLevels > 1) {
            // 3D field with vertical levels
            tempFField = inputFunctionSpace_.createField<float>(
                atlas::option::name("temperatureF") | 
                atlas::option::levels(numLevels)
            );
        } else {
            // 2D field
            tempFField = inputFunctionSpace_.createField<float>(
                atlas::option::name("temperatureF")
            );
        }
        
        // Convert temperature from Kelvin to Fahrenheit
        // Formula: °F = (K - 273.15) × 9/5 + 32
        if (numLevels > 1) {
            // 3D field: use 2D views
            auto tempView = atlas::array::make_view<float, 2>(tempField);
            auto tempFView = atlas::array::make_view<float, 2>(tempFField);
            
            for (size_t i = 0; i < spatialSize; ++i) {
                for (size_t level = 0; level < numLevels; ++level) {
                    float kelvin = tempView(i, level);
                    tempFView(i, level) = (kelvin - 273.15f) * 9.0f/5.0f + 32.0f;
                }
            }
        } else {
            // 2D field: use 1D views
            auto tempView = atlas::array::make_view<float, 1>(tempField);
            auto tempFView = atlas::array::make_view<float, 1>(tempFField);
            
            for (size_t i = 0; i < spatialSize; ++i) {
                float kelvin = tempView(i);
                tempFView(i) = (kelvin - 273.15f) * 9.0f/5.0f + 32.0f;
            }
        }
        
        // Add the Fahrenheit temperature field to fieldset
        inFs.add(tempFField);
        std::cout << "Added temperature in Fahrenheit field to fieldset" << std::endl;

        // Create new field with same structure as temperature field
        atlas::Field tempCField;
        std::cout << "Creating temperature in Celsius field..." << std::endl;
         // 3D field with vertical levels
        tempCField = inputFunctionSpace_.createField<float>(
            atlas::option::name("temperatureC") | 
            atlas::option::levels(numLevels)
        );
        auto tempView = atlas::array::make_view<float, 2>(tempField);
        auto tempCView = atlas::array::make_view<float, 2>(tempCField);
        for (size_t i = 0; i < spatialSize; ++i) {
            for (size_t level = 0; level < numLevels; ++level) {
                float kelvin = tempView(i, level);
                tempCView(i, level) = kelvin - 273.15f;
            }
        }
        // Add the Celsius temperature field to fieldset
        inFs.add(tempCField);
        std::cout << "Added temperature in Celsius field to fieldset" << std::endl;
    }

    // now let's do something random and set temperature to 177.6K at every point
    // in a box bounded by latitudes 30N to 60N and longitudes 60W to 0E (i.e., 300E to 360E)
    std::cout << "Setting temperature to 177.6K in a box from 30N to 60N and 60W to 0E..." << std::endl;
    auto tempView = atlas::array::make_view<float, 2>(tempField);
    auto coordstmp = atlas::array::make_view<double, 2>(inputFunctionSpace_.lonlat());
    for (size_t i = 0; i < spatialSize; ++i) {
        double lon = coordstmp(i, 0); // longitude is first component
        double lat = coordstmp(i, 1); // latitude is second component
        if (lat >= 30.0 && lat <= 60.0 && (lon >= 300.0)) {
            for (size_t level = 0; level < numLevels; ++level) {
                tempView(i, level) = 177.6f;
            }
        }
        if (lat >= -45.0 && lat <= -30.0 && (lon >= 300.0)) {
            for (size_t level = 0; level < numLevels; ++level) {
                tempView(i, level) = 377.6f;
            }
        }
    }

    // Create target grid and interpolation object
    
    std::cout << "Creating target grid: " << targetGridSpec << std::endl;
    
    // Create target Atlas grid from the specification
    const atlas::Grid targetGrid(targetGridSpec);
    std::cout << "Target grid created successfully" << std::endl;
    std::cout << "Target grid size: " << targetGrid.size() << " points" << std::endl;
    
    // Create target functionspace WITHOUT halo to prevent data corruption
    eckit::LocalConfiguration clean_conf; // No halo for interpolation target
    atlas::functionspace::StructuredColumns targetFunctionSpace_(targetGrid, clean_conf);
    std::cout << "Target functionspace created with " << targetFunctionSpace_.size() << " points" << std::endl;
    
    // Print some information about the target grid
    if (targetGrid.name().find("F") == 0 || targetGrid.name().find("N") == 0 || targetGrid.name().find("O") == 0) {
        std::cout << "Target grid type: Gaussian (" << targetGrid.name() << ")" << std::endl;
    } else if (targetGrid.name().find("L") == 0) {
        std::cout << "Target grid type: Regular Lat-Lon (" << targetGrid.name() << ")" << std::endl;
    } else {
        std::cout << "Target grid type: " << targetGrid.name() << std::endl;
    }

    // Interpolate temperature and temperatureF fields to target grid
    std::cout << "Creating interpolation object..." << std::endl;
    
    // Create Atlas interpolation object using bilinear method for structured grids
    atlas::Interpolation interpolation(
        atlas::option::type("structured-bilinear"),
        inputFunctionSpace_, targetFunctionSpace_
    );
    
    std::cout << "Interpolation object created successfully" << std::endl;
    
    // Create output fieldset and perform interpolation
    atlas::FieldSet outFs;
    for (const auto& field : inFs) {
        std::cout << "Interpolating field: " << field.name() << std::endl;
        
        // Create output field with same structure as input
        atlas::Field outField;
        if (numLevels > 1) {
            outField = targetFunctionSpace_.createField<float>(
                atlas::option::name(field.name()) | 
                atlas::option::levels(numLevels)
            );
        } else {
            outField = targetFunctionSpace_.createField<float>(
                atlas::option::name(field.name())
            );
        }
        
        // Perform the actual interpolation
        interpolation.execute(field, outField);
        
        outFs.add(outField);
        std::cout << "Successfully interpolated field: " << field.name() << std::endl;
    }

    // Start to write output NetCDF file
    NetCDFWriter writer(outputFile);
    writer.copyGlobalAttributes(reader.getGlobalAttributes());
    writer.addInterpolationHistory(inputFile, atlasInputGridName, targetGrid.name());

    // Get target grid info (excluding halo points)
    size_t targetGridSize = targetGrid.size(); // Use grid size, not functionspace size
    
    // Extract coordinates from target grid
    std::cout << "Extracting coordinates from target grid..." << std::endl;
    
    auto coords = atlas::array::make_view<double, 2>(targetFunctionSpace_.lonlat());
    
    // Extract unique latitudes and longitudes for structured grids
    std::vector<double> targetLats, targetLons;
    std::set<double> uniqueLats, uniqueLons;
    
    // Collect all coordinate points from target grid
    for (size_t i = 0; i < targetGridSize; ++i) {
        double lon = coords(i, 0); // longitude is first component
        double lat = coords(i, 1); // latitude is second component
        uniqueLons.insert(lon);
        uniqueLats.insert(lat);
    }
    
    // Convert sets to vectors and sort
    targetLons.assign(uniqueLons.begin(), uniqueLons.end());
    targetLats.assign(uniqueLats.begin(), uniqueLats.end());
    std::sort(targetLons.begin(), targetLons.end());
    std::sort(targetLats.begin(), targetLats.end());
    
    std::cout << "Target grid dimensions: " << targetLons.size() << " longitudes x " 
              << targetLats.size() << " latitudes" << std::endl;
    std::cout << "Target latitude range: " << targetLats.front() << " to " << targetLats.back() << std::endl;
    std::cout << "Target longitude range: " << targetLons.front() << " to " << targetLons.back() << std::endl;
    
    // Create NetCDF dimensions for target grid
    auto lonDim = writer.addDimension("longitude", targetLons.size());
    auto latDim = writer.addDimension("latitude", targetLats.size());
    
    // Add level dimension if we have vertical levels
    netCDF::NcDim levelDim;
    if (numLevels > 1) {
        levelDim = writer.addDimension("level", numLevels);
    }
    
    // Create coordinate variables in NetCDF file
    auto lonVar = writer.addVariable("longitude", netCDF::ncDouble, {lonDim});
    auto latVar = writer.addVariable("latitude", netCDF::ncDouble, {latDim});
    
    // Write coordinate data
    writer.writeVariableData("longitude", targetLons);
    writer.writeVariableData("latitude", targetLats);
    
    std::cout << "Created NetCDF coordinate variables and wrote coordinate data" << std::endl;

    // Write interpolated fields to NetCDF file
    for (const auto& field : outFs) {
        std::cout << "Writing field to NetCDF: " << field.name() << std::endl;
        
        // Create variable in NetCDF file with proper dimension ordering
        std::vector<netCDF::NcDim> varDims;
        if (numLevels > 1) {
            varDims = {levelDim, latDim, lonDim}; // 3D field: level, lat, lon (C order)
        } else {
            varDims = {latDim, lonDim}; // 2D field: lat, lon
        }
        auto var = writer.addVariable(field.name(), netCDF::ncFloat, varDims);
        
        // Get the total expected output size
        size_t expectedSize = targetLons.size() * targetLats.size();
        if (numLevels > 1) expectedSize *= numLevels;
        
        // Copy data from Atlas field (should match exactly since no halo)
        std::vector<float> fieldData;
        
        if (numLevels > 1) {
            // 3D field: copy data directly from Atlas 2D view
            auto fieldView = atlas::array::make_view<float, 2>(field);
            fieldData.resize(expectedSize);
            
            // Since we're using clean functionspace, sizes should match exactly
            size_t idx = 0;
            for (size_t level = 0; level < numLevels; ++level) {
                for (size_t i = 0; i < targetGridSize; ++i) {
                    fieldData[idx] = fieldView(i, level);
                    idx++;
                }
            }
        } else {
            // 2D field: copy data directly from Atlas 1D view
            auto fieldView = atlas::array::make_view<float, 1>(field);
            fieldData.resize(expectedSize);
            
            for (size_t i = 0; i < targetGridSize; ++i) {
                fieldData[i] = fieldView(i);
            }
        }
        
        // Print min/max values for output Atlas field
        auto minVal = *std::min_element(fieldData.begin(), fieldData.end());
        auto maxVal = *std::max_element(fieldData.begin(), fieldData.end());
        
        writer.writeVariableData(field.name(), fieldData);
        std::cout << "Successfully wrote field: " << field.name() << " with dimensions " 
                  << targetLons.size() << "x" << targetLats.size();
        if (numLevels > 1) std::cout << "x" << numLevels;
        std::cout << " | Range: [" << minVal << ", " << maxVal << "]" << std::endl;
    }
}