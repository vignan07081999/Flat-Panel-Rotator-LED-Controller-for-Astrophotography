cmake_minimum_required(VERSION 3.10)  # Adjust version as needed
project(indi_flatfield)

# Find the INDI library
find_package(INDI REQUIRED)

# Add the executable
add_executable(indi_flatfield indi_flatfield.cpp)

# Link against the INDI library
target_link_libraries(indi_flatfield ${INDI_LIBRARIES})

# Install the driver
install(TARGETS indi_flatfield DESTINATION lib/indi)
