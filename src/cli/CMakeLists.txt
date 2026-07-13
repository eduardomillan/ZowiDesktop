# src/cli/CMakeLists.txt
add_executable(zowi_cli main.cpp)

target_link_libraries(zowi_cli PRIVATE
    Zowi::core
    Zowi::bt_qt
    Zowi::bt_serial
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Bluetooth
    CLI11::CLI11
)

# Grant CAP_NET_ADMIN so the CLI can bind RFCOMM / flash firmware without sudo.
# Applied on every (re)build because the binary is overwritten and loses the
# capability. Uses `sudo -n` (non-interactive) so a build without cached sudo
# credentials does not hang or fail; it just prints a reminder instead.
find_program(SETCAP_PROGRAM setcap)
if(SETCAP_PROGRAM)
    add_custom_command(TARGET zowi_cli POST_BUILD
        COMMAND bash -c "sudo -n ${SETCAP_PROGRAM} cap_net_admin+ep '$<TARGET_FILE:zowi_cli>' 2>/dev/null || echo 'NOTE: could not auto-apply CAP_NET_ADMIN (run: sudo setcap cap_net_admin+ep $<TARGET_FILE:zowi_cli>)'"
        COMMENT "Granting CAP_NET_ADMIN to zowi_cli (needs sudo)"
        VERBATIM)
endif()

