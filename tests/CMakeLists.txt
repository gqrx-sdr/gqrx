###############################################################################
# TX modulator test: runs the modulators in a GNU Radio flow graph with
# vector source and sink blocks. No hardware or audio device is needed.
add_executable(tx_modulator_test
    tx_modulator_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_modulator.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_util.cpp
)
set_property(TARGET tx_modulator_test PROPERTY CXX_STANDARD 17)
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
set_property(TARGET tx_util_test PROPERTY CXX_STANDARD 17)
target_include_directories(tx_util_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME tx_util_test COMMAND tx_util_test)

###############################################################################
# More pure C++ TX tests, one per area.
foreach(t tx_dcs_test tx_rigctl_ptt_test tx_power_mic_gain_test
          tx_device_usrp_hackrf_plutosdr_test)
    add_executable(${t} ${t}.cpp ${CMAKE_SOURCE_DIR}/src/dsp/tx_util.cpp)
    set_property(TARGET ${t} PROPERTY CXX_STANDARD 17)
    target_include_directories(${t} PRIVATE ${CMAKE_SOURCE_DIR}/src)
    add_test(NAME ${t} COMMAND ${t})
endforeach()

# TX telemetry test with a fake provider. Pure C++.
add_executable(tx_telemetry_test
    tx_telemetry_test.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_telemetry.cpp
)
set_property(TARGET tx_telemetry_test PROPERTY CXX_STANDARD 17)
target_include_directories(tx_telemetry_test PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME tx_telemetry_test COMMAND tx_telemetry_test)
