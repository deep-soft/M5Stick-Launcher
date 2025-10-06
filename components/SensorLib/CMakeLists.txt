set(SENSORLIB_URL https://github.com/lewisxhe/SensorLib.git)
set(SENSORLIB_DIR ${CMAKE_CURRENT_BINARY_DIR}/SensorLib)
if(NOT EXISTS ${SENSORLIB_DIR})
  execute_process(COMMAND git clone --depth 1 ${SENSORLIB_URL} ${SENSORLIB_DIR})
endif()
file(GLOB SENSORLIB_SOURCES ${SENSORLIB_DIR}/src/*.*)
idf_component_register(SRCS ${SENSORLIB_SOURCES}
                      INCLUDE_DIRS ${SENSORLIB_DIR}/src
                      REQUIRES arduino-esp32)
