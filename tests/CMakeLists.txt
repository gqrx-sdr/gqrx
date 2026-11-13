set(TEST_SRCS
    test_tx.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/tx_modulator.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/rx_filter.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/rx_meter.cpp
    ${CMAKE_SOURCE_DIR}/src/dsp/resampler_xx.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/gqrx/transmitter.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/gqrx/tx_device_args.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/gqrx/remote_control.cpp
)

if(DEFINED LINUX_AUDIO_BACKEND AND LINUX_AUDIO_BACKEND MATCHES "Pulseaudio")
    list(APPEND TEST_SRCS ${CMAKE_SOURCE_DIR}/src/pulseaudio/pa_source.cc)
endif()

add_executable(test_tx ${TEST_SRCS})

if(Qt6_FOUND)
    set_property(TARGET test_tx PROPERTY CXX_STANDARD 17)
    target_link_libraries(test_tx
        Qt6::Core
        Qt6::Network
        Qt6::Widgets
    )
else()
    set_property(TARGET test_tx PROPERTY CXX_STANDARD 14)
    target_link_libraries(test_tx
        Qt5::Core
        Qt5::Network
        Qt5::Widgets
    )
endif()

target_include_directories(test_tx PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/include
    ${GNURADIO_OSMOSDR_INCLUDE_DIRS}
)

if(NOT Gnuradio_VERSION VERSION_LESS "3.10")
    target_link_libraries(test_tx
        ${GNURADIO_OSMOSDR_LIBRARIES}
        gnuradio::gnuradio-analog
        gnuradio::gnuradio-blocks
        gnuradio::gnuradio-filter
        gnuradio::gnuradio-audio
        gnuradio::gnuradio-runtime
        gnuradio::gnuradio-network
        Volk::volk
    )
else()
    target_link_libraries(test_tx
        ${GNURADIO_OSMOSDR_LIBRARIES}
        gnuradio::gnuradio-analog
        gnuradio::gnuradio-blocks
        gnuradio::gnuradio-filter
        gnuradio::gnuradio-audio
        gnuradio::gnuradio-runtime
        Volk::volk
    )
endif()

if(DEFINED LINUX_AUDIO_BACKEND AND LINUX_AUDIO_BACKEND MATCHES "Pulseaudio")
    target_link_libraries(test_tx
        ${PULSEAUDIO_LIBRARY}
        ${PULSE-SIMPLE}
    )
endif()

if(SoapySDR_FOUND)
    target_link_libraries(test_tx SoapySDR)
endif()

add_test(NAME test_tx COMMAND test_tx)

# Device-string check for the TX probe. Needs no GNU Radio, Qt or hardware.
add_executable(test_tx_device_args
    test_tx_device_args.cpp
    ${CMAKE_SOURCE_DIR}/src/applications/gqrx/tx_device_args.cpp
)
set_property(TARGET test_tx_device_args PROPERTY CXX_STANDARD 14)
target_include_directories(test_tx_device_args PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME test_tx_device_args COMMAND test_tx_device_args)
