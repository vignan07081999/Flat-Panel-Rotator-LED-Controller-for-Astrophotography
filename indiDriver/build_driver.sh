#!/bin/bash

set -e  # Exit immediately if a command exits with a non-zero status

echo "Starting build process for INDI FlatField driver..."

# --- Dependency Checks (Robust) ---

check_package() {
    local package_name=$1
    echo "Checking for $package_name..."
    if ! dpkg -s "$package_name" &> /dev/null; then
        echo "$package_name is not installed or in a bad state. Attempting to install/reinstall..."
        sudo apt-get update
        sudo apt-get install --reinstall -y "$package_name"
        if ! dpkg -s "$package_name" &> /dev/null; then  # Double-check after install
             echo "ERROR: Failed to install/reinstall $package_name.  Exiting."
             exit 1
        fi

    else
        echo "$package_name is installed."
    fi
}
check_package cmake
check_package libindi-dev
check_package libudev-dev

# --- Clean Build ---
echo "Cleaning up previous build (if any)..."
rm -rf build

# --- Create Build Directory ---
echo "Creating build directory..."
mkdir build
cd build

# --- Run CMake ---
echo "Running CMake..."
cmake ..
if [ $? -ne 0 ]; then
    echo "ERROR: CMake failed.  Exiting."
    exit 1
fi

# --- Compile the Driver ---
echo "Compiling the driver..."
make
if [ $? -ne 0 ]; then
    echo "ERROR: Compilation failed.  Exiting."
    exit 1
fi
# --- Install the Driver ---
echo "Installing the driver..."
sudo make install
if [ $? -ne 0 ]; then
    echo "ERROR: Installation failed.  Exiting."
    exit 1
fi

echo "Driver built and installed successfully!"
