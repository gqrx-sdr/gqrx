###############################################################################
# TX modulator test: runs the modulators in a GNU Radio flow graph with
# vector source and sink blocks. No hardware or audio device is needed.
add_executable(tx_modulator_test
    tx_modulator_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_modulator.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_util.cpp
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

###############################################################################
# TX helper test: device string, split offset and CTCSS table. Pure C++.
add_executable(tx_util_test
    tx_util_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_util.cpp
)
if(Qt6_FOUND)
    set_property(TARGET tx_util_test PROPERTY CXX_STANDARD 17)
else()
    set_property(TARGET tx_util_test PROPERTY CXX_STANDARD 14)
endif()
target_include_directories(tx_util_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME tx_util_test COMMAND tx_util_test)
