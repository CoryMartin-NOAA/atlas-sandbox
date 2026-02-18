#!/usr/bin/env python3
"""
Plot interpolated temperature fields from Atlas NetCDF output
"""

import numpy as np
import matplotlib.pyplot as plt
import cartopy.crs as ccrs
import cartopy.feature as cfeature
from netCDF4 import Dataset
import os

def plot_temperature_maps(filename):
    """
    Create global maps of interpolated temperature fields
    
    Parameters:
    filename (str): Path to the NetCDF file
    """
    
    # Check if file exists
    if not os.path.exists(filename):
        print(f"Error: File {filename} not found")
        return
    
    # Open NetCDF file
    print(f"Opening NetCDF file: {filename}")
    with Dataset(filename, 'r') as nc:
        # Read coordinates
        lons = nc.variables['longitude'][:]
        lats = nc.variables['latitude'][:]
        
        # Read temperature data (level, lat, lon)
        temp_k = nc.variables['temperature'][:]
        temp_f = nc.variables['temperatureF'][:]
        
        print(f"Data dimensions: {temp_k.shape}")
        print(f"Longitude range: {lons.min():.2f} to {lons.max():.2f}")
        print(f"Latitude range: {lats.min():.2f} to {lats.max():.2f}")
        print(f"Number of levels: {temp_k.shape[0]}")
        
        # Print temperature statistics
        print(f"Temperature (K) range: [{temp_k.min():.2f}, {temp_k.max():.2f}]")
        print(f"Temperature (F) range: [{temp_f.min():.2f}, {temp_f.max():.2f}]")
    
    # Create meshgrid for plotting
    lon_grid, lat_grid = np.meshgrid(lons, lats)
    
    # Select representative levels for plotting
    # Note: Vertical coordinate goes from model top (level 0) to surface (level shape[0]-1)
    levels_to_plot = [
        (0, "Top Level (Level 0)"),
        (temp_k.shape[0]//4, f"Upper Atmosphere (Level {temp_k.shape[0]//4})"),
        (temp_k.shape[0]//2, f"Mid Atmosphere (Level {temp_k.shape[0]//2})"),
        (temp_k.shape[0]//4 * 3, f"Mid Troposphere (Level {temp_k.shape[0]//4 * 3})"),
        (-1, f"Surface (Level {temp_k.shape[0]-1})")
    ]
    
    # Create figure with subplots
    fig = plt.figure(figsize=(20, 20))
    
    # Plot each selected level
    for i, (level_idx, title_suffix) in enumerate(levels_to_plot):
        # Temperature in Kelvin
        ax1 = fig.add_subplot(5, 2, 2*i+1, projection=ccrs.PlateCarree())
        ax1.set_global()
        ax1.coastlines()
        ax1.add_feature(cfeature.BORDERS)
        ax1.add_feature(cfeature.OCEAN, alpha=0.5)
        ax1.add_feature(cfeature.LAND, alpha=0.3)
        
        # Plot temperature in Kelvin
        temp_plot = temp_k[level_idx, :, :]
        im1 = ax1.contourf(lon_grid, lat_grid, temp_plot, 
                          levels=20, cmap='coolwarm', extend='both',
                          transform=ccrs.PlateCarree())
        ax1.set_title(f'Temperature (K) - {title_suffix}\nRange: [{temp_plot.min():.1f}, {temp_plot.max():.1f}]K')
        ax1.gridlines(draw_labels=True, alpha=0.5)
        
        # Add colorbar
        plt.colorbar(im1, ax=ax1, shrink=0.8, label='Temperature (K)')
        
        # Temperature in Fahrenheit
        ax2 = fig.add_subplot(5, 2, 2*i+2, projection=ccrs.PlateCarree())
        ax2.set_global()
        ax2.coastlines()
        ax2.add_feature(cfeature.BORDERS)
        ax2.add_feature(cfeature.OCEAN, alpha=0.5)
        ax2.add_feature(cfeature.LAND, alpha=0.3)
        
        # Plot temperature in Fahrenheit
        temp_f_plot = temp_f[level_idx, :, :]
        im2 = ax2.contourf(lon_grid, lat_grid, temp_f_plot,
                          levels=20, cmap='coolwarm', extend='both', 
                          transform=ccrs.PlateCarree())
        ax2.set_title(f'Temperature (°F) - {title_suffix}\nRange: [{temp_f_plot.min():.1f}, {temp_f_plot.max():.1f}]°F')
        ax2.gridlines(draw_labels=True, alpha=0.5)
        
        # Add colorbar
        plt.colorbar(im2, ax=ax2, shrink=0.8, label='Temperature (°F)')
    
    plt.tight_layout()
    
    # Save the plot
    output_file = filename.replace('.nc', '_temperature_maps.png')
    plt.savefig(output_file, dpi=150, bbox_inches='tight')
    print(f"Saved temperature maps to: {output_file}")
    
    # Show the plot
    plt.show()

def plot_vertical_profile(filename, lon_idx=None, lat_idx=None):
    """
    Plot vertical temperature profile at a specific location
    
    Parameters:
    filename (str): Path to the NetCDF file
    lon_idx (int): Longitude index (default: center)
    lat_idx (int): Latitude index (default: center)
    """
    
    with Dataset(filename, 'r') as nc:
        temp_k = nc.variables['temperature'][:]
        temp_f = nc.variables['temperatureF'][:]
        lons = nc.variables['longitude'][:]
        lats = nc.variables['latitude'][:]
        
        # Use center point if not specified
        if lon_idx is None:
            lon_idx = len(lons) // 2
        if lat_idx is None:
            lat_idx = len(lats) // 2
        
        # Extract vertical profile
        temp_profile_k = temp_k[:, lat_idx, lon_idx]
        temp_profile_f = temp_f[:, lat_idx, lon_idx]
        
        levels = np.arange(temp_k.shape[0])
        
        # Create vertical profile plot
        fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 8))
        
        # Temperature in Kelvin
        ax1.plot(temp_profile_k, levels, 'b-o', linewidth=2, markersize=4)
        ax1.set_xlabel('Temperature (K)')
        ax1.set_ylabel('Level Index')
        ax1.set_title(f'Vertical Temperature Profile (K)\nLon: {lons[lon_idx]:.2f}°, Lat: {lats[lat_idx]:.2f}°')
        ax1.grid(True, alpha=0.3)
        ax1.invert_yaxis()  # Invert y-axis so surface is at bottom
        
        # Temperature in Fahrenheit
        ax2.plot(temp_profile_f, levels, 'r-o', linewidth=2, markersize=4)
        ax2.set_xlabel('Temperature (°F)')
        ax2.set_ylabel('Level Index')
        ax2.set_title(f'Vertical Temperature Profile (°F)\nLon: {lons[lon_idx]:.2f}°, Lat: {lats[lat_idx]:.2f}°')
        ax2.grid(True, alpha=0.3)
        ax2.invert_yaxis()
        
        plt.tight_layout()
        
        # Save the plot
        output_file = filename.replace('.nc', '_vertical_profile.png')
        plt.savefig(output_file, dpi=150, bbox_inches='tight')
        print(f"Saved vertical profile to: {output_file}")
        
        plt.show()

def main():
    """Main function to plot temperature maps and vertical profiles"""
    
    # File path
    nc_file = "test.nc"
    
    print("=== Atlas Temperature Interpolation Visualization ===")
    
    if not os.path.exists(nc_file):
        print(f"Error: NetCDF file '{nc_file}' not found in current directory")
        print("Make sure you're running this script from the atlas-sandbox directory")
        return
    
    # Plot temperature maps at different levels
    print("\n1. Creating temperature maps...")
    plot_temperature_maps(nc_file)
    
    # Plot vertical temperature profile
    print("\n2. Creating vertical temperature profile...")
    plot_vertical_profile(nc_file)
    
    print("\nVisualization complete!")

if __name__ == "__main__":
    main()