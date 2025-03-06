cmake_minimum_required(VERSION 3.10)
project(indi_flatfield)

# Use pkg-config to find INDI
find_package(PkgConfig REQUIRED)
pkg_check_modules(INDI REQUIRED libindi>=1.7) # Ensure you have a recent INDI

# Include INDI headers
include_directories(${INDI_INCLUDE_DIRS})

add_executable(indi_simpleflat indi_simpleflat.cpp)

# Link against INDI libraries
target_link_libraries(indi_simpleflat ${INDI_LIBRARIES})

install(TARGETS indi_simpleflat DESTINATION lib/indi)
