
include_directories(./
    ${CMAKE_SOURCE_DIR}/mqtt/include)

add_executable(firmware main.c)

target_link_libraries(firmware
    main
    net80211
    wpa
    lwip
    pp
    phy
    gcc
    mqtt
    ssl
)

set_target_properties(firmware PROPERTIES
    LINK_FLAGS "-L${ESP_SDK_BASE}/ld -Teagle.app.v6.ld")


set(FW_ADDR_1 0x00000)
set(FW_ADDR_2 0x40000)

add_custom_target(firmware_binary ALL
    COMMAND esptool.py elf2image firmware
)

set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES 
    "firmware-${FW_ADDR_1}.bin;firmware-${FW_ADDR_2}.bin")

add_dependencies(firmware_binary firmware)

add_custom_target(flash 
    COMMAND esptool.py write_flash
        0x00000 firmware-${FW_ADDR_1}.bin
        0x40000 firmware-${FW_ADDR_2}.bin
)

add_dependencies(flash firmware_binary)

