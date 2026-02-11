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
    
    std::string latDimName = reader.getLatitudeDimName();
    std::string lonDimName = reader.getLongitudeDimName();
    
    // create an atlas functionspace
    std::string atlasInputGridName;
    atlasInputGridName = "F" + std::to_string(lats.size()/2); // Assuming regular Gaussian grid with 2x latitudes for total points
    const atlas::Grid gridInput(atlasInputGridName);

    // Create atlas fieldset from the input data

    // Calculate and add a new field (e.g., temperature in Celsius from Kelvin)

    // Create target grid and interpolation object

    // Perform interpolation for each variable requested

    // Write output to NetCDF file (not shown here, but you would use NetCDF C++ API to write the data from the atlas fields)
}