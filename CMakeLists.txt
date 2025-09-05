# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

# Include board specific sources before defining the project
list(APPEND EXTRA_COMPONENT_DIRS boards/m5stack-cardputer)

set(COMPONENTS src)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Launcher)
