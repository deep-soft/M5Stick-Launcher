# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.16)

include(board_map.cmake)
if(PIO_BOARD_DIR)
  list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/boards/${PIO_BOARD_DIR})
endif()
list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/src)
list(APPEND EXTRA_COMPONENT_DIRS ${CMAKE_SOURCE_DIR}/components)

execute_process(
  COMMAND python3 ${CMAKE_SOURCE_DIR}/parse_pio_flags.py ${PIO_ENV_NAME} ${PIO_BOARD_DIR}
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE PIO_FLAG_OUTPUT
  OUTPUT_STRIP_TRAILING_WHITESPACE)
string(REGEX MATCH "FLAGS=([^\n]*)" _ "${PIO_FLAG_OUTPUT}")
set(PIO_FLAGS "${CMAKE_MATCH_1}")
string(REGEX MATCH "TARGET=([^\n]*)" _ "${PIO_FLAG_OUTPUT}")
  set(PIO_TARGET "${CMAKE_MATCH_1}")
  string(REGEX MATCH "PARTITIONS=([^\n]*)" _ "${PIO_FLAG_OUTPUT}")
  set(PIO_PARTITIONS "${CMAKE_MATCH_1}")
  string(REGEX MATCH "SDKCONFIG=([^\n]*)" _ "${PIO_FLAG_OUTPUT}")
  set(PIO_SDKCONFIG "${CMAKE_MATCH_1}")
  if(PIO_TARGET)
    set(IDF_TARGET ${PIO_TARGET})
  endif()
  if(PIO_PARTITIONS)
    file(CREATE_LINK ${CMAKE_SOURCE_DIR}/${PIO_PARTITIONS} ${CMAKE_SOURCE_DIR}/partitions.csv SYMBOLIC)
  endif()
  set(SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/sdkconfig.defaults)
  if(PIO_SDKCONFIG)
    list(APPEND SDKCONFIG_DEFAULTS ${CMAKE_SOURCE_DIR}/build/${PIO_SDKCONFIG})
  endif()

  include($ENV{IDF_PATH}/tools/cmake/project.cmake)

if(PIO_FLAGS)
  foreach(opt ${PIO_FLAGS})
    if(opt MATCHES "^-D(.+)")
      idf_build_set_property(COMPILE_DEFINITIONS "${CMAKE_MATCH_1}" APPEND)
    else()
      idf_build_set_property(COMPILE_OPTIONS "${opt}" APPEND)
    endif()
  endforeach()
  file(WRITE ${CMAKE_BINARY_DIR}/applied_build_flags.log "${PIO_FLAGS}\n")
endif()

idf_build_set_property(C_COMPILE_OPTIONS "-include${CMAKE_SOURCE_DIR}/boards/pinouts/pins_arduino.h" APPEND)
idf_build_set_property(CXX_COMPILE_OPTIONS "-include${CMAKE_SOURCE_DIR}/boards/pinouts/pins_arduino.h" APPEND)

project(Launcher)
