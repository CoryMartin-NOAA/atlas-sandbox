#!/usr/bin/env python3
"""
Example script to download a GDAS file and run interpolation
This demonstrates how to use the atlas_interpolate tool
"""

import os
import sys
import subprocess
from datetime import datetime, timedelta
import urllib.request

def download_gdas_file(date_str, cycle, forecast_hour, output_dir="."):
    """
    Download a GDAS atmospheric file from NOMADS
    
    Args:
        date_str: Date in YYYYMMDD format
        cycle: Model cycle (00, 06, 12, 18)
        forecast_hour: Forecast hour (e.g., 006)
        output_dir: Directory to save the file
    
    Returns:
        Path to downloaded file or None if failed
    """
    base_url = "https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod"
    filename = f"gdas.t{cycle}z.atmf{forecast_hour}.nc"
    url = f"{base_url}/gdas.{date_str}/{cycle}/atmos/{filename}"
    
    output_path = os.path.join(output_dir, filename)
    
    print(f"Downloading: {url}")
    print(f"Saving to: {output_path}")
    
    try:
        urllib.request.urlretrieve(url, output_path)
        print(f"✓ Download complete: {output_path}")
        return output_path
    except Exception as e:
        print(f"✗ Download failed: {e}")
        return None

def run_interpolation(netcdf_file, target_grid, output_file, variable=None):
    """
    Run the atlas_interpolate program
    
    Args:
        netcdf_file: Path to NetCDF file
        target_grid: Target grid specification (e.g., 'O32', 'N32', 'L360x181')
        output_file: Path to output NetCDF file
        variable: Optional variable name to interpolate (default: all x,y and x,y,z variables)
    """
    exe_path = "./build/atlas_interpolate"
    
    if not os.path.exists(exe_path):
        print(f"Error: Executable not found at {exe_path}")
        print("Please build the project first using: ./build.sh")
        return False
    
    print(f"\nRunning interpolation:")
    print(f"  Input: {netcdf_file}")
    print(f"  Target grid: {target_grid}")
    print(f"  Output: {output_file}")
    if variable:
        print(f"  Variable: {variable}")
    else:
        print(f"  Mode: Interpolate all x,y and x,y,z variables")
    print()
    
    cmd = [exe_path, netcdf_file, target_grid, output_file]
    if variable:
        cmd.append(variable)
    
    try:
        subprocess.run(cmd, check=True)
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Interpolation failed with exit code {e.returncode}")
        return False
    except Exception as e:
        print(f"✗ Error running interpolation: {e}")
        return False

def main():
    print("=== GDAS Interpolation Example ===")
    print()
    
    # Default parameters (use yesterday's data as it's more likely to be available)
    yesterday = datetime.now() - timedelta(days=1)
    date_str = yesterday.strftime("%Y%m%d")
    cycle = "00"
    forecast_hour = "006"
    
    # Check if file already exists
    filename = f"gdas.t{cycle}z.atmf{forecast_hour}.nc"
    
    if os.path.exists(filename):
        print(f"Using existing file: {filename}")
        netcdf_file = filename
    else:
        print("Attempting to download sample GDAS file...")
        print(f"Date: {date_str}, Cycle: {cycle}Z, Forecast: F{forecast_hour}")
        print()
        
        netcdf_file = download_gdas_file(date_str, cycle, forecast_hour)
        
        if not netcdf_file:
            print()
            print("Download failed. This could be because:")
            print("  1. The file is not yet available on the server")
            print("  2. The file has been archived")
            print("  3. Network connectivity issues")
            print()
            print("You can manually download a file from:")
            print("  https://nomads.ncep.noaa.gov/pub/data/nccf/com/gfs/prod/")
            print()
            print("Then run:")
            print(f"  ./build/atlas_interpolate <your_file.nc> O32 output.nc")
            return 1
    
    # Example 1: Interpolate to O32 grid (Octahedral reduced Gaussian)
    print("\n" + "="*60)
    print("Example 1: Interpolate all variables to O32 (Octahedral Gaussian grid)")
    print("="*60)
    success = run_interpolation(netcdf_file, "O32", "gdas_o32.nc")
    
    if not success:
        return 1
    
    # Example 2: Interpolate to a regular lat-lon grid
    print("\n" + "="*60)
    print("Example 2: Interpolate only temperature to L360x181 (1-degree lat-lon grid)")
    print("="*60)
    success = run_interpolation(netcdf_file, "L360x181", "gdas_latlon.nc", "tmp")
    
    if not success:
        return 1
    
    print("\n" + "="*60)
    print("✓ All examples completed successfully!")
    print("="*60)
    
    return 0

if __name__ == "__main__":
    sys.exit(main())
