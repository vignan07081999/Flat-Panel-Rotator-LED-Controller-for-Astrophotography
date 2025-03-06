cmake_minimum_required(VERSION 3.10)
project(indi-rotationpanel)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- INDI Configuration (Manual) ---
set(INDI_INCLUDE_DIR "/usr/include")
set(INDI_LIBRARIES indi indidriver)
set(INDI_LIBRARY_DIR "/usr/lib")  # OR /usr/lib/arm-linux-gnueabihf - VERIFY!

# --- Qt5 Configuration ---
set(Qt5SerialPort_DIR "/usr/lib/arm-linux-gnueabihf/cmake/Qt5SerialPort")  # Your Qt5SerialPort path
find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED)

if(NOT Qt5Core_FOUND)
    message(FATAL_ERROR "Qt5Core not found!")
endif()

if (NOT Qt5SerialPort_FOUND)
 message(FATAL_ERROR "Qt5SerialPort not found.")
endif()

# --- Include Directories ---
include_directories(
    ${INDI_INCLUDE_DIR}
    ${Qt5Core_INCLUDE_DIRS}
    ${Qt5SerialPort_INCLUDE_DIRS}
)

# --- Add Executable ---
add_executable(indi_rotationpanel RotationPanel.cpp)

# --- Link Libraries ---
link_directories(${INDI_LIBRARY_DIR})
target_link_libraries(indi_rotationpanel
    ${INDI_LIBRARIES}
    Qt5::Core
    Qt5::SerialPort
)

# --- Installation ---
install(TARGETS indi_rotationpanel DESTINATION lib/indi)
