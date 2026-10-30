add_executable(gqrx_tests
    test_main.cpp
    ${CMAKE_SOURCE_DIR}/src/interfaces/audio_recorder_f.cpp
    ${CMAKE_SOURCE_DIR}/src/interfaces/udp_sink_f.cpp
)

target_include_directories(gqrx_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(gqrx_tests PRIVATE
    gnuradio::gnuradio-runtime
    gnuradio::gnuradio-blocks
)

if(Qt6_FOUND)
    set_property(TARGET gqrx_tests PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET gqrx_tests PROPERTY CXX_STANDARD 14)
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Linux|FreeBSD")
    find_program(OBJCOPY objcopy)
    if(OBJCOPY)
        add_custom_command(TARGET gqrx_tests POST_BUILD
            COMMAND ${OBJCOPY} --remove-section .note.gnu.property $<TARGET_FILE:gqrx_tests>
            COMMENT "Stripping note.gnu.property for CPU compatibility"
        )
    endif()
endif()

add_test(NAME gqrx_tests COMMAND gqrx_tests)
