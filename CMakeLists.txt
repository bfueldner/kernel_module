set(CMAKE_VERBOSE_MAKEFILE "ON")

set(TOOLCHAIN_PREFIX "/opt/gcc-linaro-arm-linux-gnueabihf-4.8-2014.02_linux")

set(TARGET_TRIPLET "arm-linux-gnueabihf")
#set(TARGET_MODE "thumb")
#set(TARGET_CPU "cortex-m4")

set(CMAKE_TOOLCHAIN_FILE "arm_toolchain.cmake")

cmake_minimum_required(VERSION 2.8)
project(kmodule C)

# Startup parameters
#add_definitions(-D__STACK_SIZE=3072)
#add_definitions(-D__HEAP_SIZE=4096)
#add_definitions(-D__STARTUP_COPY_MULTIPLE)
#add_definitions(-D__STARTUP_CLEAR_BSS_MULTIPLE)
#add_definitions(-D__STARTUP_CLEAR_BSS)
#add_definitions(-D__NO_SYSTEM_INIT)
#add_definitions(-D__START=_start)

#set(LINK_NANO "--specs=nano.specs")
#set(LINK_SEMIHOST "--specs=rdimon.specs")
#set(LINK_NOHOST "--specs=nosys.specs")

#set(CMAKE_C_FLAGS   "-m${TARGET_MODE} -mcpu=${TARGET_CPU} -ffunction-sections -fdata-sections" CACHE INTERNAL "C Compiler Flags")
#set(CMAKE_CXX_FLAGS "-m${TARGET_MODE} -mcpu=${TARGET_CPU} -ffunction-sections -fdata-sections -fno-rtti -fno-exceptions -std=c++11" CACHE INTERNAL "C++ Compiler Flags")
#set(CMAKE_ASM_FLAGS "-m${TARGET_MODE} -mcpu=${TARGET_CPU}" CACHE INTERNAL "Assembler Compiler Flags")
#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -T ${CMAKE_CURRENT_SOURCE_DIR}/TM4C123.ld ${LINK_NANO} ${LINK_NOHOST} -Wl,--gc-sections,-Map=${PROJECT_NAME}.map")

include_directories("src")
include_directories("/opt/PHYTEC_BSPs/phyCORE-AM335x-PD13.1.0/platform-phyCORE-AM335x/build-target/linux-3.2/include")

add_executable(kmodule
src/morse.c
)

#add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_SIZE} ${CMAKE_BINARY_DIR}/${PROJECT_NAME})
