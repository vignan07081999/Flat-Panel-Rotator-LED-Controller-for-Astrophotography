#!/bin/bash

# Check if required packages are installed
if ! command -v cmake &> /dev/null; then
    echo "cmake is not installed.  Installing..."
    sudo apt-get update
    sudo apt-get install -y cmake
fi

if ! dpkg -s libindi-dev &> /dev/null; then
    echo "libindi-dev is not installed.  Installing..."
    sudo apt-get update
    sudo apt-get install -y libindi-dev
fi

if ! dpkg -s libudev-dev &> /dev/null; then
    echo "libudev-dev is not installed.  Installing..."
    sudo apt-get update
    sudo apt-get install -y libudev-dev
fi

# Create build directory if it doesn't exist
if [ ! -d "build" ]; then
  mkdir build
fi

# Change to build directory
cd build

# Run CMake
cmake ..

# Compile the driver
make

# Install the driver
sudo make install

# Clean up (optional - removes the build directory)
# cd ..
# rm -rf build

echo "Driver built and installed successfully!"
