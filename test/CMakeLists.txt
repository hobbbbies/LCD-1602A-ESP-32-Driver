# This is the project CMakeLists.txt file for the test subproject
cmake_minimum_required(VERSION 3.5)

# Set the components to include the tests for.
# This can be overridden from CMake cache:
# - when invoking CMake directly: cmake -D TEST_COMPONENTS="xxxxx" ..
#
set(TEST_COMPONENTS "app_main" CACHE STRING "List of components to test")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
# "Trim" the build. Include the minimal set of components, main, and anything it depends on.
idf_build_set_property(MINIMAL_BUILD ON)
project(LCD_DRIVER_test)