cmake_minimum_required(VERSION 3.0)
project(flatfield_driver)

# Find the INDI library
find_package(INDI REQUIRED)

# Include INDI headers
include_directories(${INDI_INCLUDE_DIRS})

# Add the executable
add_executable(flatfield_driver flatfield_driver.cpp)

# Link against the INDI library
target_link_libraries(flatfield_driver ${INDI_LIBRARIES})

# Install the driver (optional, but recommended)
install(TARGETS flatfield_driver DESTINATION lib/indi)

# Create an XML file for the driver (important for INDI to recognize it)
configure_file(
    flatfield_driver.xml.in
    ${CMAKE_BINARY_DIR}/flatfield_driver.xml
)
install(FILES ${CMAKE_BINARY_DIR}/flatfield_driver.xml DESTINATION share/indi)
