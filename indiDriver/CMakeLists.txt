cmake_minimum_required(VERSION 3.10)
project(indi_flatfield)

list(APPEND CMAKE_MODULE_PATH "/home/stellarmate/Projects/indi/cmake_modules/FindINDI.cmake") # YOUR PATH HERE

find_package(INDI REQUIRED)
find_package(PkgConfig REQUIRED)
pkg_check_modules(UDEV REQUIRED libudev)

add_executable(indi_flatfield indi_flatfield.cpp)

target_link_libraries(indi_flatfield ${INDI_LIBRARIES} ${UDEV_LIBRARIES})

install(TARGETS indi_flatfield DESTINATION lib/indi)
