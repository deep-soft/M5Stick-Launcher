set(XPOWER_URL https://github.com/lewisxhe/XPowersLib.git)
set(XPOWER_DIR ${CMAKE_CURRENT_BINARY_DIR}/XPowerLib)
if(NOT EXISTS ${XPOWER_DIR})
  execute_process(COMMAND git clone --depth 1 ${XPOWER_URL} ${XPOWER_DIR})
endif()
file(GLOB XPOWER_SOURCES ${XPOWER_DIR}/src/*.*)
idf_component_register(SRCS ${XPOWER_SOURCES}
                      INCLUDE_DIRS ${XPOWER_DIR}/src
                      REQUIRES arduino-esp32)
