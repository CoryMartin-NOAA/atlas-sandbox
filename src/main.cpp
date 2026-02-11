#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <cmath>
#include <netcdf>
#include "atlas/grid.h"
#include "atlas/mesh.h"
#include "atlas/meshgenerator.h"
#include "atlas/functionspace.h"
#include "atlas/field.h"
#include "atlas/array.h"
#include "atlas/interpolation.h"
#include "atlas/option.h"
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
        std::vector<std::string> latNames = {"lat", "latitude", "grid_yt"};
        
        for (const auto& name : latNames) {
            if (hasVariable(name)) {
                netCDF::NcVar latVar = ncfile_->getVar(name);
                std::vector<double> lats(latVar.getDim(0).getSize());
                latVar.getVar(lats.data());
                std::cout << "Read " << lats.size() << " latitude values from '" << name << "'" << std::endl;
                return lats;
            }
        }
        
        std::cerr << "Could not find latitude variable" << std::endl;
        return {};
    }

    std::vector<double> readLongitudes() {
        // Try common longitude variable names
        std::vector<std::string> lonNames = {"lon", "longitude", "grid_xt"};
        
        for (const auto& name : lonNames) {
            if (hasVariable(name)) {
                netCDF::NcVar lonVar = ncfile_->getVar(name);
                std::vector<double> lons(lonVar.getDim(0).getSize());
                lonVar.getVar(lons.data());
                std::cout << "Read " << lons.size() << " longitude values from '" << name << "'" << std::endl;
                return lons;
            }
        }
        
        std::cerr << "Could not find longitude variable" << std::endl;
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

private:
    std::string filename_;
    std::unique_ptr<netCDF::NcFile> ncfile_;
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
        auto meshConfig = util::Config("partitioner", "equal_regions");
        sourceMesh_ = MeshGenerator("structured").generate(sourceGrid_, meshConfig);
        
        std::cout << "Generating target mesh..." << std::endl;
        targetMesh_ = MeshGenerator("structured").generate(targetGrid_, meshConfig);
        
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
    std::cout << "Usage: " << progName << " <netcdf_file> <target_grid> [variable_name]" << std::endl;
    std::cout << "\nArguments:" << std::endl;
    std::cout << "  netcdf_file    : Path to NetCDF file to read" << std::endl;
    std::cout << "  target_grid    : Target grid specification (e.g., 'O32', 'N32', 'L360x181')" << std::endl;
    std::cout << "  variable_name  : Optional variable to interpolate (default: 'tmp')" << std::endl;
    std::cout << "\nGrid specifications:" << std::endl;
    std::cout << "  O<n>     : Octahedral reduced Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  N<n>     : Regular Gaussian grid with <n> latitude lines" << std::endl;
    std::cout << "  L<nx>x<ny> : Regular lat-lon grid with <nx> longitudes and <ny> latitudes" << std::endl;
    std::cout << "\nExample:" << std::endl;
    std::cout << "  " << progName << " gdas.t00z.atmf006.nc O32 tmp" << std::endl;
}

int main(int argc, char* argv[]) {
    std::cout << "=== Atlas NetCDF Interpolation Example ===" << std::endl;
    
    // Parse command line arguments
    if (argc < 3) {
        printUsage(argv[0]);
        return 1;
    }
    
    std::string inputFile = argv[1];
    std::string targetGridSpec = argv[2];
    std::string variableName = (argc >= 4) ? argv[3] : "tmp";
    
    std::cout << "\nConfiguration:" << std::endl;
    std::cout << "  Input file: " << inputFile << std::endl;
    std::cout << "  Target grid: " << targetGridSpec << std::endl;
    std::cout << "  Variable: " << variableName << std::endl;
    
    try {
        // Read NetCDF file
        NetCDFReader reader(inputFile);
        reader.printInfo();
        
        // Read coordinates
        auto lats = reader.readLatitudes();
        auto lons = reader.readLongitudes();
        
        if (lats.empty() || lons.empty()) {
            std::cerr << "Error: Could not read coordinates from NetCDF file" << std::endl;
            return 1;
        }
        
        // Read variable
        auto data = reader.readVariable(variableName);
        if (data.empty()) {
            std::cerr << "Error: Could not read variable '" << variableName << "'" << std::endl;
            return 1;
        }
        
        // Setup and perform interpolation
        AtlasInterpolator interpolator(lats, lons, targetGridSpec);
        interpolator.printTargetGridInfo();
        
        auto interpolatedData = interpolator.interpolate(data);
        
        std::cout << "\n=== SUCCESS ===" << std::endl;
        std::cout << "Successfully interpolated " << data.size() 
                  << " source points to " << interpolatedData.size() 
                  << " target points" << std::endl;
        
        return 0;
        
    } catch (const std::exception& e) {
        std::cerr << "\nError: " << e.what() << std::endl;
        return 1;
    }
}
