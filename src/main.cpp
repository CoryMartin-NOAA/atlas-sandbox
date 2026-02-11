#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <map>
#include <netcdf>
#include "atlas/grid.h"
#include "atlas/mesh.h"
#include "atlas/meshgenerator.h"
#include "atlas/functionspace.h"
#include "atlas/field.h"
#include "atlas/array.h"
#include "atlas/interpolation.h"
#include "atlas/option.h"
#include "atlas/library/Library.h"
#include "eckit/log/Log.h"

using namespace atlas;

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

class AtlasInterpolator {
public:
    AtlasInterpolator(const std::vector<double>& srcLats,
                      const std::vector<double>& srcLons,
                      const std::string& targetGridSpec) 
        : srcLats_(srcLats), srcLons_(srcLons) {
        
        std::cout << "\n=== Setting up Atlas Interpolation ===" << std::endl;
        
        // Create source grid (structured lat-lon from input file)
        std::cout << "Creating source grid from input data..." << std::endl;
        sourceGrid_ = createSourceGrid();
        
        // Create target grid from specification
        std::cout << "Creating target grid: " << targetGridSpec << std::endl;
        targetGrid_ = Grid(targetGridSpec);
        
        // Create meshes and function spaces
        std::cout << "Generating source mesh..." << std::endl;
        sourceMesh_ = MeshGenerator("structured").generate(sourceGrid_);
        
        std::cout << "Generating target mesh..." << std::endl;
        targetMesh_ = MeshGenerator("structured").generate(targetGrid_);
        
        // Create function spaces
        sourceFunctionSpace_ = functionspace::NodeColumns(sourceMesh_);
        targetFunctionSpace_ = functionspace::NodeColumns(targetMesh_);
        
        std::cout << "Source grid points: " << sourceFunctionSpace_.size() << std::endl;
        std::cout << "Target grid points: " << targetFunctionSpace_.size() << std::endl;
        
        // Create interpolation
        std::cout << "Setting up interpolation scheme..." << std::endl;
        interpolation_ = Interpolation(
            option::type("structured-linear2D"),
            sourceFunctionSpace_,
            targetFunctionSpace_
        );
        
        std::cout << "Interpolation ready!" << std::endl;
    }

    std::vector<double> interpolate(const std::vector<float>& sourceData) {
        std::cout << "\n=== Performing Interpolation ===" << std::endl;
        
        // Create source field
        auto sourceField = sourceFunctionSpace_.createField<double>(
            option::name("source_field")
        );
        
        // Copy data to field
        auto sourceView = array::make_view<double, 1>(sourceField);
        size_t minSize = std::min(sourceData.size(), (size_t)sourceView.size());
        for (size_t i = 0; i < minSize; ++i) {
            sourceView(i) = static_cast<double>(sourceData[i]);
        }
        
        // Create target field
        auto targetField = targetFunctionSpace_.createField<double>(
            option::name("target_field")
        );
        
        // Execute interpolation
        std::cout << "Executing interpolation..." << std::endl;
        interpolation_.execute(sourceField, targetField);
        
        // Extract results
        auto targetView = array::make_view<double, 1>(targetField);
        std::vector<double> result(targetView.size());
        for (idx_t i = 0; i < targetView.size(); ++i) {
            result[i] = targetView(i);
        }
        
        std::cout << "Interpolation complete! Output size: " << result.size() << std::endl;
        
        // Print some statistics
        printStatistics(sourceData, result);
        
        return result;
    }

    std::vector<double> getTargetLatitudes() {
        auto lonlat = array::make_view<double, 2>(targetMesh_.nodes().lonlat());
        std::vector<double> lats(lonlat.shape(0));
        for (idx_t i = 0; i < lonlat.shape(0); ++i) {
            lats[i] = lonlat(i, 1);
        }
        return lats;
    }

    std::vector<double> getTargetLongitudes() {
        auto lonlat = array::make_view<double, 2>(targetMesh_.nodes().lonlat());
        std::vector<double> lons(lonlat.shape(0));
        for (idx_t i = 0; i < lonlat.shape(0); ++i) {
            lons[i] = lonlat(i, 0);
        }
        return lons;
    }

    size_t getTargetGridSize() const {
        return targetFunctionSpace_.size();
    }

    std::string getTargetGridName() const {
        return targetGrid_.name();
    }

    void printTargetGridInfo() {
        std::cout << "\n=== Target Grid Information ===" << std::endl;
        std::cout << "Grid name: " << targetGrid_.name() << std::endl;
        std::cout << "Grid points: " << targetFunctionSpace_.size() << std::endl;
        
        auto lonlat = array::make_view<double, 2>(targetMesh_.nodes().lonlat());
        if (lonlat.size() > 0) {
            std::cout << "First point: (lon=" << lonlat(0, 0) 
                      << ", lat=" << lonlat(0, 1) << ")" << std::endl;
            if (lonlat.shape(0) > 1) {
                std::cout << "Last point: (lon=" << lonlat(lonlat.shape(0)-1, 0) 
                          << ", lat=" << lonlat(lonlat.shape(0)-1, 1) << ")" << std::endl;
            }
        }
    }

private:
    std::vector<double> srcLats_;
    std::vector<double> srcLons_;
    Grid sourceGrid_;
    Grid targetGrid_;
    Mesh sourceMesh_;
    Mesh targetMesh_;
    functionspace::NodeColumns sourceFunctionSpace_;
    functionspace::NodeColumns targetFunctionSpace_;
    Interpolation interpolation_;

    Grid createSourceGrid() {
        // For structured grids, create a grid spec
        // Assuming regular lat-lon grid
        idx_t nlat = srcLats_.size();
        idx_t nlon = srcLons_.size();
        
        std::string gridSpec = "L" + std::to_string(nlon) + "x" + std::to_string(nlat);
        std::cout << "Source grid spec: " << gridSpec << std::endl;
        
        return Grid(gridSpec);
    }

    void printStatistics(const std::vector<float>& source, 
                        const std::vector<double>& target) {
        // Compute basic statistics
        double sourceMin = std::numeric_limits<double>::max();
        double sourceMax = std::numeric_limits<double>::lowest();
        double sourceSum = 0.0;
        
        for (const auto& val : source) {
            if (std::isfinite(val)) {
                sourceMin = std::min(sourceMin, (double)val);
                sourceMax = std::max(sourceMax, (double)val);
                sourceSum += val;
            }
        }
        
        double targetMin = std::numeric_limits<double>::max();
        double targetMax = std::numeric_limits<double>::lowest();
        double targetSum = 0.0;
        
        for (const auto& val : target) {
            if (std::isfinite(val)) {
                targetMin = std::min(targetMin, val);
                targetMax = std::max(targetMax, val);
                targetSum += val;
            }
        }
        
        std::cout << "\nStatistics:" << std::endl;
        std::cout << "  Source - Min: " << sourceMin << ", Max: " << sourceMax 
                  << ", Mean: " << sourceSum / source.size() << std::endl;
        std::cout << "  Target - Min: " << targetMin << ", Max: " << targetMax 
                  << ", Mean: " << targetSum / target.size() << std::endl;
    }
};

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <netcdf_file> <target_grid> <output_file> [variable_name]" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  netcdf_file    : Path to NetCDF file to read" << std::endl;
    std::cout << "  target_grid    : Target grid specification (e.g., 'O32', 'N32', 'L360x181')" << std::endl;
    std::cout << "  output_file    : Path to output NetCDF file" << std::endl;
    std::cout << "  variable_name  : Optional specific variable to interpolate (default: all x,y and x,y,z variables)" << std::endl;
    std::cout << "\nGrid specifications:" << std::endl;
    std::cout << "  O<n>     : Octahedral reduced Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  N<n>     : Regular Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  L<nx>x<ny> : Regular lat-lon grid with <nx> longitudes and <ny> latitudes" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << progName << " gdas.t00z.atmf006.nc O32 gdas_interpolated.nc" << std::endl;
    std::cout << "  " << progName << " gdas.t00z.atmf006.nc L360x181 output.nc tmp" << std::endl;
}

int main(int argc, char* argv[]) {
    // Initialize Atlas library
    atlas::Library::instance().initialise(argc, argv);
    
    std::cout << "=== Atlas NetCDF Interpolation Example ===" << std::endl;
    
    // Parse command line arguments
    if (argc < 4) {
        printUsage(argv[0]);
        atlas::Library::instance().finalise();
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string targetGridSpec = argv[2];
    std::string outputFile = argv[3];
    std::string specificVariable = (argc >= 5) ? argv[4] : "";
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Input file: " << inputFile << std::endl;
    std::cout << "  Target grid: " << targetGridSpec << std::endl;
    std::cout << "  Output file: " << outputFile << std::endl;
    if (!specificVariable.empty()) {
        std::cout << "  Specific variable: " << specificVariable << std::endl;
    } else {
        std::cout << "  Mode: Interpolate all x,y and x,y,z variables" << std::endl;
    }
    
    try {
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
        
        std::string latDimName = reader.getLatitudeDimName();
        std::string lonDimName = reader.getLongitudeDimName();
        
        // Setup interpolation
        AtlasInterpolator interpolator(lats, lons, targetGridSpec);
        interpolator.printTargetGridInfo();
        
        // Create output file
        NetCDFWriter writer(outputFile);
        
        // Copy global attributes
        writer.copyGlobalAttributes(reader.getGlobalAttributes());
        
        // Add interpolation history
        std::string originalResolution = std::to_string(lons.size()) + "x" + std::to_string(lats.size());
        writer.addInterpolationHistory(inputFile, originalResolution, targetGridSpec);
        
        // Get target grid info
        size_t targetGridSize = interpolator.getTargetGridSize();
        auto targetLats = interpolator.getTargetLatitudes();
        auto targetLons = interpolator.getTargetLongitudes();
        
        std::cout << "\n=== Creating Output File Structure ===" << std::endl;
        
        // Get all variables and dimensions from input
        auto allVars = reader.getAllVariables();
        auto allDims = reader.getAllDimensions();
        
        // Create dimensions in output file
        std::map<std::string, netCDF::NcDim> outputDims;
        
        // Create a single grid_points dimension for unstructured output
        outputDims["grid_points"] = writer.addDimension("grid_points", targetGridSize);
        
        // Copy non-spatial dimensions
        for (const auto& dim : allDims) {
            if (dim.first != latDimName && dim.first != lonDimName) {
                outputDims[dim.first] = writer.addDimension(dim.first, dim.second.getSize());
                std::cout << "  Copied dimension: " << dim.first << " (size: " << dim.second.getSize() << ")" << std::endl;
            }
        }
        
        // Create coordinate variables
        auto latVar = writer.addVariable("lat", netCDF::ncDouble, {outputDims["grid_points"]});
        latVar.putAtt("units", "degrees_north");
        latVar.putAtt("long_name", "latitude");
        latVar.putAtt("standard_name", "latitude");
        writer.writeVariableData("lat", targetLats);
        
        auto lonVar = writer.addVariable("lon", netCDF::ncDouble, {outputDims["grid_points"]});
        lonVar.putAtt("units", "degrees_east");
        lonVar.putAtt("long_name", "longitude");
        lonVar.putAtt("standard_name", "longitude");
        writer.writeVariableData("lon", targetLons);
        
        std::cout << "  Created coordinate variables (lat, lon)" << std::endl;
        
        // Identify and interpolate variables
        std::cout << "\n=== Interpolating Variables ===" << std::endl;
        
        std::vector<std::string> variablesToInterpolate;
        
        if (!specificVariable.empty()) {
            // Only interpolate the specific variable
            variablesToInterpolate.push_back(specificVariable);
        } else {
            // Find all variables with x,y or x,y,z dimensions
            for (const auto& var : allVars) {
                const std::string& varName = var.first;
                
                // Skip coordinate variables
                if (varName == latDimName || varName == lonDimName || 
                    varName == "lat" || varName == "lon" ||
                    varName == "latitude" || varName == "longitude") {
                    continue;
                }
                
                auto dims = var.second.getDims();
                
                // Check if variable has x,y dimensions (2D or 3D with vertical)
                bool hasLatDim = false;
                bool hasLonDim = false;
                
                for (const auto& dim : dims) {
                    if (dim.getName() == latDimName) hasLatDim = true;
                    if (dim.getName() == lonDimName) hasLonDim = true;
                }
                
                if (hasLatDim && hasLonDim) {
                    variablesToInterpolate.push_back(varName);
                }
            }
        }
        
        std::cout << "Variables to interpolate: " << variablesToInterpolate.size() << std::endl;
        for (const auto& varName : variablesToInterpolate) {
            std::cout << "  - " << varName << std::endl;
        }
        
        // Interpolate each variable
        for (const auto& varName : variablesToInterpolate) {
            std::cout << "\n--- Interpolating variable: " << varName << " ---" << std::endl;
            
            auto sourceVar = reader.getVariable(varName);
            auto sourceDims = sourceVar.getDims();
            
            // Build output dimensions for this variable
            std::vector<netCDF::NcDim> outDims;
            std::vector<size_t> extraDimSizes;
            
            for (const auto& dim : sourceDims) {
                if (dim.getName() == latDimName || dim.getName() == lonDimName) {
                    // Replace with grid_points dimension (only add once)
                    if (outDims.empty() || outDims.back().getName() != "grid_points") {
                        outDims.push_back(outputDims["grid_points"]);
                    }
                } else {
                    // Keep other dimensions
                    outDims.push_back(outputDims[dim.getName()]);
                    extraDimSizes.push_back(dim.getSize());
                }
            }
            
            // Create variable in output file
            auto outputVar = writer.addVariable(varName, sourceVar.getType(), outDims);
            writer.copyVariableAttributes(sourceVar, varName);
            
            // Read and interpolate data
            if (extraDimSizes.empty()) {
                // Simple 2D variable
                auto sourceData = reader.readVariable(varName);
                auto interpolatedData = interpolator.interpolate(sourceData);
                writer.writeVariableData(varName, interpolatedData);
            } else {
                // 3D or higher dimensional variable - interpolate each 2D slice
                std::cout << "  Interpolating multi-dimensional variable..." << std::endl;
                
                // Calculate total size and slice sizes
                size_t totalSourceSize = 1;
                for (const auto& dim : sourceDims) {
                    totalSourceSize *= dim.getSize();
                }
                
                size_t sourceSliceSize = lats.size() * lons.size();
                size_t numSlices = totalSourceSize / sourceSliceSize;
                
                std::cout << "  Number of 2D slices: " << numSlices << std::endl;
                
                // Read all data
                auto allData = reader.readVariable(varName);
                
                // Interpolate each slice
                std::vector<double> allInterpolatedData;
                allInterpolatedData.reserve(numSlices * targetGridSize);
                
                // Allocate sliceData once and reuse for all slices
                std::vector<float> sliceData(sourceSliceSize);
                
                for (size_t slice = 0; slice < numSlices; ++slice) {
                    size_t offset = slice * sourceSliceSize;
                    std::copy(allData.begin() + offset, 
                              allData.begin() + offset + sourceSliceSize,
                              sliceData.begin());
                    
                    auto interpolatedSlice = interpolator.interpolate(sliceData);
                    allInterpolatedData.insert(allInterpolatedData.end(),
                                               interpolatedSlice.begin(),
                                               interpolatedSlice.end());
                }
                
                writer.writeVariableData(varName, allInterpolatedData);
                std::cout << "  Wrote " << allInterpolatedData.size() << " interpolated values" << std::endl;
            }
        }
        
        std::cout << "\n=== SUCCESS ===" << std::endl;
        std::cout << "Successfully interpolated " << variablesToInterpolate.size() 
                  << " variable(s) from " << inputFile << std::endl;
        std::cout << "Output written to: " << outputFile << std::endl;
        
        atlas::Library::instance().finalise();
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        atlas::Library::instance().finalise();
        return 1;
    }
}
