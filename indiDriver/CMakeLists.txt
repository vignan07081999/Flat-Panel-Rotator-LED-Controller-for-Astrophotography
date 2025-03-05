cmake_minimum_required(VERSION 3.5)
project(indi_flatpanel)

find_package(INDI REQUIRED)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(indi_flatpanel_driver indi_flatpanel_driver.cpp)

target_link_libraries(indi_flatpanel_driver PRIVATE indidriver)
install(TARGETS indi_flatpanel_driver RUNTIME DESTINATION bin)
