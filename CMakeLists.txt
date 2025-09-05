# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

set(COMPONENTS src)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(Launcher)
list(APPEND EXTRA_COMPONENT_DIRS boars/m5stack-cardputer)
