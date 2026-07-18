# src/cli/CMakeLists.txt
qt6_add_resources(CLI_I18N_RC ${CMAKE_SOURCE_DIR}/i18n.qrc)
add_executable(zowi_cli
    main.cpp
    cli_state.cpp
    cli_util.cpp
    cli_commands.cpp
)

target_link_libraries(zowi_cli PRIVATE
    Zowi::core
    Zowi::bt_qt
    Zowi::firmware
    Qt${ZOWI_QT_VERSION}::Bluetooth
    CLI11::CLI11
)
target_sources(zowi_cli PRIVATE ${CLI_I18N_RC})

if(NOT WIN32)
    target_link_libraries(zowi_cli PRIVATE Zowi::bt_serial)
endif()

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

