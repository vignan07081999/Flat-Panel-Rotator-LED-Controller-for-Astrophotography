cmake_minimum_required(VERSION 3.10)
project(indi_flatfield)

find_package(INDI REQUIRED)
find_package(PkgConfig REQUIRED) # Find PkgConfig
pkg_check_modules(UDEV REQUIRED libudev) # Use PkgConfig to find libudev

add_executable(indi_flatfield indi_flatfield.cpp)

target_link_libraries(indi_flatfield ${INDI_LIBRARIES} ${UDEV_LIBRARIES}) # Link against INDI and libudev

install(TARGETS indi_flatfield DESTINATION lib/indi)
