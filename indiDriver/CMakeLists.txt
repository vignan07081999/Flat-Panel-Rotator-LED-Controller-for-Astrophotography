cmake_minimum_required(VERSION 3.10)
project(indi_flatfield)

# Add common INDI CMake module paths.  This is a "best guess" approach.
# If this doesn't work, we'll need to find the *exact* path to FindINDI.cmake.
list(APPEND CMAKE_MODULE_PATH
    /usr/share/cmake/Modules  # Very common location
    /usr/share/cmake-3.16/Modules # Another common location (version may vary)
    /usr/share/cmake-3.18/Modules # Another common location (version may vary)
     /usr/share/cmake-3.20/Modules # Another common location (version may vary)
    /usr/lib/x86_64-linux-gnu/cmake/Modules # Possible location on some systems
    /usr/local/lib/cmake/indi #Less Common Location
    /opt/indi/cmake/Modules
)

find_package(INDI REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED libudev)

add_executable(indi_flatfield indi_flatfield.cpp)

target_link_libraries(indi_flatfield ${INDI_LIBRARIES} ${UDEV_LIBRARIES})

install(TARGETS indi_flatfield DESTINATION lib/indi)
