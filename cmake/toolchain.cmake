include(CMakeForceCompiler)

set(CMAKE_SYSTEM_NAME esp8266)

set(ESP_OPEN_SDK_PATH "/home/yura/esp/esp-open-sdk")
set(ESP_SDK_BASE "${ESP_OPEN_SDK_PATH}/sdk")
set(TOOLCHAIN_BIN_PATH "${ESP_OPEN_SDK_PATH}/xtensa-lx106-elf/bin")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}")

CMAKE_FORCE_C_COMPILER("${TOOLCHAIN_BIN_PATH}/xtensa-lx106-elf-gcc" GNU_C)
CMAKE_FORCE_CXX_COMPILER("${TOOLCHAIN_BIN_PATH}/xtensa-lx106-elf-g++" GNU_CXX)

set(CMAKE_C_FLAGS "-std=gnu99 -Wpointer-arith -Wno-implicit-function-declaration \
    -Wundef -pipe -D__ets__ -DICACHE_FLASH -fno-inline-functions \
    -ffunction-sections -nostdlib -mlongcalls -mtext-section-literals \
    -falign-functions=4 -fdata-sections"  CACHE STRING "")

set(CMAKE_C_FLAGS_DEBUG " -Og -ggdb3 -DUSE_GDBSTUB" 
    CACHE STRING "")

set(CMAKE_C_FLAGS_RELEASE " -Os -g" 
    CACHE STRING "")

set(CMAKE_CXX_FLAGS "-D__ets__ -DICACHE_FLASH -mlongcalls \
    -mtext-section-literals -fno-exceptions -fno-rtti -falign-functions=4 \
    -std=c++11 -MMD -ffunction-sections -fdata-sections" CACHE STRING "")

set(CMAKE_CXX_FLAGS_DEBUG "-Og -ggdb3 -DUSE_GDBSTUB" 
    CACHE STRING "")

set(CMAKE_CXX_FLAGS_RELEASE "-Os -g" 
    CACHE STRING "")

set(CMAKE_EXE_LINKER_FLAGS "-nostdlib -Wl,--no-check-sections \
    -Wl,-static -Wl,--gc-sections")

set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> \
    <LINK_FLAGS> -o <TARGET> \
    -Wl,--start-group <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group") 

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_CXX_COMPILER> <FLAGS> \
    <CMAKE_CXX_LINK_FLAGS> <LINK_FLAGS> -o <TARGET> \
    -Wl,--start-group <OBJECTS> <LINK_LIBRARIES> -Wl,--end-group")
