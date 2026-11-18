###############################################################################
# Test suite; enable with -DGQRX_BUILD_TESTS=ON
option(GQRX_BUILD_TESTS "Build the gqrx test suite" OFF)

if(GQRX_BUILD_TESTS)
    add_executable(test_tx
        test_tx.cpp
        ../src/dsp/tx_mod_nfm.cpp
        ../src/dsp/tx_mod_am.cpp
        ../src/dsp/tx_mod_ssb.cpp
    )

    target_include_directories(test_tx PRIVATE ${CMAKE_SOURCE_DIR}/src)
    set_property(TARGET test_tx PROPERTY CXX_STANDARD 17)

    if(Qt6_FOUND)
        target_link_libraries(test_tx Qt6::Core)
    else()
        target_link_libraries(test_tx Qt5::Core)
    endif()

    target_link_libraries(test_tx
        ${GNURADIO_OSMOSDR_LIBRARIES}
        gnuradio::gnuradio-analog
        gnuradio::gnuradio-blocks
        gnuradio::gnuradio-filter
        Volk::volk
    )

    enable_testing()
    add_test(NAME tx COMMAND test_tx)
endif()
