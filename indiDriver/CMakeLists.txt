cmake_minimum_required(VERSION 3.10)
project(indi-rotationpanel)

set(CMAKE_CXX_STANDARD 11)  # Or a later standard if needed (14, 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find INDI
find_package(INDI REQUIRED)
include_directories(${INDI_INCLUDE_DIRS})
link_directories(${INDI_LIBRARY_DIRS})

# Find Qt5
find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED) #Important
include_directories(${Qt5Core_INCLUDE_DIRS} ${Qt5SerialPort_INCLUDE_DIRS})


add_executable(indi_rotationpanel RotationPanel.cpp)

target_link_libraries(indi_rotationpanel ${INDI_LIBRARIES} Qt5::Core Qt5::SerialPort)

# Install the driver
install(TARGETS indi_rotationpanel DESTINATION lib/indi)
