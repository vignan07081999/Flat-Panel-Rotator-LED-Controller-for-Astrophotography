cmake_minimum_required(VERSION 3.10)
project(indi_simpleflat)

# Find INDI.  You might need to adjust CMAKE_MODULE_PATH as before if this fails.
find_package(INDI REQUIRED)

add_executable(indi_simpleflat indi_simpleflat.cpp)

target_link_libraries(indi_simpleflat ${INDI_LIBRARIES})

install(TARGETS indi_simpleflat DESTINATION lib/indi)
