cmake_minimum_required(VERSION 3.10)
project(indi-rotationpanel)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Find INDI (Use find_package, and THAT'S IT) ---
find_package(INDI REQUIRED)

# --- Qt5 Configuration ---
set(Qt5SerialPort_DIR "/usr/lib/arm-linux-gnueabihf/cmake/Qt5SerialPort")  # Your Qt5SerialPort path (EXAMPLE!).
find_package(Qt5Core REQUIRED)
find_package(Qt5SerialPort REQUIRED)

if(NOT Qt5Core_FOUND)
    message(FATAL_ERROR "Qt5Core not found!")
endif()

if (NOT Qt5SerialPort_FOUND)
 message(FATAL_ERROR "Qt5SerialPort not found.")
endif()

# --- Add Executable ---
add_executable(indi_rotationpanel RotationPanel.cpp)

# --- Link Libraries (Use the INDI::indi target!) ---
target_link_libraries(indi_rotationpanel
    INDI::indi  # This is the KEY CHANGE. Use the INDI target.
    Qt5::Core
    Qt5::SerialPort
)

# --- Installation ---
install(TARGETS indi_rotationpanel DESTINATION lib/indi)
