add_executable(test_iq_bridge
    test_iq_bridge.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/iq_bridge.cpp
)
target_include_directories(test_iq_bridge PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(test_iq_bridge gnuradio::gnuradio-blocks)
add_test(NAME iq_bridge COMMAND test_iq_bridge)
