###############################################################################
# TX modulator test: runs the modulators in a GNU Radio flow graph with
# vector source and sink blocks. No hardware or audio device is needed.
add_executable(tx_modulator_test
    tx_modulator_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_modulator.cpp
)
if(Qt6_FOUND)
    set_property(TARGET tx_modulator_test PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET tx_modulator_test PROPERTY CXX_STANDARD 14)
endif()
target_include_directories(tx_modulator_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(tx_modulator_test
    gnuradio::gnuradio-analog
    gnuradio::gnuradio-blocks
    gnuradio::gnuradio-filter
    Volk::volk
)
add_test(NAME tx_modulator_test COMMAND tx_modulator_test)
