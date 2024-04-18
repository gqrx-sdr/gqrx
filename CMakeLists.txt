cmake_minimum_required(VERSION 3.2)

# Project name
project(gqrx)
set(${PROJECT_NAME}_MAJOR "2")
set(${PROJECT_NAME}_MINOR "17")
set(${PROJECT_NAME}_PATCH "5")
set(IS_RELEASE FALSE)

if(IS_RELEASE)
    if(${PROJECT_NAME}_PATCH EQUAL 0)
        set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}")
    else()
        set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}.${${PROJECT_NAME}_PATCH}")
    endif()
else()
    execute_process(
        COMMAND git describe --long --dirty
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        OUTPUT_VARIABLE VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
endif()
add_definitions(-DVERSION="${VERSION}")
file(WRITE ${CMAKE_BINARY_DIR}/version.txt ${VERSION})

set(PACKAGE ${PROJECT_NAME})

########### Main global variables ###########
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build, options are: Debug GProf Valgrind Release" FORCE)
endif()

set(BUILDTYPE ${CMAKE_BUILD_TYPE})
string(TOUPPER ${BUILDTYPE} BUILDTYPE)
add_definitions(-D${BUILDTYPE})

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules)

# Add valgrind build options if necessary
if(${CMAKE_BUILD_TYPE} MATCHES "Valgrind")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
endif()

# Workaround for naming collision with log4cpp for Debug builds
add_definitions(-DLOG4CPP_FIX_ERROR_COLLISION=1)

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    add_compile_options(-Wall)
    add_compile_options(-Wextra)
    add_compile_options(-Wdouble-promotion)
    add_compile_options(-Wno-unused-parameter)
    add_compile_options(-Wsign-compare)
endif()

if(MSVC)
    #get math definitions like M_PI
    add_definitions(-D_USE_MATH_DEFINES)

    #use std::min()/std::max()
    add_definitions(-DNOMINMAX)

    #needed to dynamically link boost
    add_definitions(-DBOOST_ALL_DYN_LINK)

    #export gr-rds symbols
    add_definitions(-Dgnuradio_RDS_EXPORTS)

    if ("${MSVC_VERSION}" VERSION_LESS "1900")
        add_definitions(-D__func__=__FUNCTION__)
    endif()
endif()

# Functions & macros.  These must be defined before including subdirectories.

# function to collect all the sources from sub-directories
# into a single list
function(add_source_files list)
    get_property(is_defined GLOBAL PROPERTY SRCS_LIST DEFINED)
    if(NOT is_defined)
        define_property(GLOBAL PROPERTY ${list}
            BRIEF_DOCS "List of source files"
            FULL_DOCS "List of source files to be compiled in one library")
    endif()
    # make absolute paths
    set(SRCS)
    foreach(s IN LISTS ARGN)
        if(NOT IS_ABSOLUTE "${s}")
            get_filename_component(s "${s}" ABSOLUTE)
        endif()
        list(APPEND SRCS "${s}")
    endforeach()
    # append to global list
    set_property(GLOBAL APPEND PROPERTY ${list} "${SRCS}")
endfunction(add_source_files)

if(APPLE AND EXISTS /usr/local/opt/qt5)
    # Homebrew installs Qt5 (up to at least 5.9.1) in
    # /usr/local/opt/qt5, ensure it can be found by CMake since
    # it is not in the default /usr/local prefix.
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt5")
endif()

if(APPLE AND EXISTS /usr/local/opt/qt6)
    list(APPEND CMAKE_PREFIX_PATH "/usr/local/opt/qt6")
endif()

if(APPLE AND EXISTS /usr/local/opt/icu4c/lib)
    # With Homebrew icu4c, `-licudata -licui18n -licuuc`
    # are required, which reside in /usr/local/opt/icu4c/lib.
    # See src/CMakeFiles/gqrx.dir/link.txt in the build directory.
    set(ICU4C_LIBRARY_DIRS "/usr/local/opt/icu4c/lib")
else()
    set(ICU4C_LIBRARY_DIRS "")
endif()

# 3rd Party Dependency Stuff
option(FORCE_QT6 "Force Qt6 to be used" OFF)
option(FORCE_QT5 "Force Qt5 to be used" OFF)

if(FORCE_QT6)
    find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets Svg SvgWidgets)
elseif(FORCE_QT5)
    find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets Svg)
else()
    find_package(Qt6 QUIET COMPONENTS Core Network Widgets Svg SvgWidgets)
    if(NOT Qt6_FOUND)
        find_package(Qt5 REQUIRED COMPONENTS Core Network Widgets Svg)
    endif()
endif()

include(FindPkgConfig)
find_package(Gnuradio-osmosdr REQUIRED)

set(GR_REQUIRED_COMPONENTS RUNTIME ANALOG AUDIO BLOCKS DIGITAL FILTER FFT PMT)
find_package(Gnuradio REQUIRED COMPONENTS analog audio blocks digital filter fft network)
if(NOT Gnuradio_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to compile gqrx")
endif()

find_package(Volk)

# Pass the GNU Radio version as 0xMMNNPP BCD.
math(EXPR GNURADIO_BCD_VERSION
    "(${Gnuradio_VERSION_MAJOR} / 10) << 20 |
     (${Gnuradio_VERSION_MAJOR} % 10) << 16 |
     (${Gnuradio_VERSION_MINOR} / 10) << 12 |
     (${Gnuradio_VERSION_MINOR} % 10) <<  8 |
     (${Gnuradio_VERSION_PATCH} / 10) <<  4 |
     (${Gnuradio_VERSION_PATCH} % 10) <<  0
    "
)
add_definitions(-DGNURADIO_VERSION=${GNURADIO_BCD_VERSION})

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux|FreeBSD")
    if(NOT LINUX_AUDIO_BACKEND)
        set(LINUX_AUDIO_BACKEND Pulseaudio CACHE STRING "Choose the audio backend, options are: Pulseaudio, Portaudio, Gr-audio" FORCE)
    endif()

    if(${LINUX_AUDIO_BACKEND} MATCHES "Pulseaudio")
        message(STATUS "Pulseaudio backend enabled")
        find_package(PulseAudio REQUIRED)
        # there is a defect in the pulse audio cmake file that does not include this library. So we add it here.
        find_library(PULSE-SIMPLE NAMES pulse-simple REQUIRED)
        add_definitions(-DWITH_PULSEAUDIO)
        unset(PORTAUDIO_INCLUDE_DIRS CACHE)
        unset(PORTAUDIO_LIBRARIES CACHE)
    elseif(${LINUX_AUDIO_BACKEND} MATCHES "Portaudio")
        message(STATUS "Portaudio backend enabled")
        find_package(PORTAUDIO REQUIRED)
        add_definitions(-DWITH_PORTAUDIO)
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
    elseif(${LINUX_AUDIO_BACKEND} MATCHES "Gr-audio")
        message(STATUS "Gr-audio backend enabled")
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
        unset(PORTAUDIO_INCLUDE_DIRS CACHE)
        unset(PORTAUDIO_LIBRARIES CACHE)
    else()
        message(FATAL_ERROR "Invalid audio backend: should be either Pulseaudio, Portaudio or Gr-audio")
    endif()
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
    if(NOT OSX_AUDIO_BACKEND)
        set(OSX_AUDIO_BACKEND Portaudio CACHE STRING "Choose the audio backend, options are: Portaudio and Gr-audio" FORCE)
    endif()

    if(${OSX_AUDIO_BACKEND} MATCHES "Portaudio")
        message(STATUS "Portaudio backend enabled")
        find_package(PORTAUDIO REQUIRED)
        add_definitions(-DWITH_PORTAUDIO)
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
    elseif(${OSX_AUDIO_BACKEND} MATCHES "Gr-audio")
        message(STATUS "Gr-audio backend enabled")
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
        unset(PORTAUDIO_INCLUDE_DIRS CACHE)
        unset(PORTAUDIO_LIBRARIES CACHE)
    else()
        message(FATAL_ERROR "Invalid audio backend: should be either Portaudio or Gr-audio")
    endif()
endif()

find_program(CCACHE "ccache")
if(CCACHE)
    set(CMAKE_C_COMPILER_LAUNCHER ${CCACHE})
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE})
    set(ENV{CCACHE_SLOPPINESS} pch_defines,time_macros)
endif(CCACHE)


# Airspy optimizations that require modified gr-osmosdr
option(CUSTOM_AIRSPY_KERNELS "Enable non-standard Airspy optimizations" ON)
if(CUSTOM_AIRSPY_KERNELS)
    add_definitions(-DCUSTOM_AIRSPY_KERNELS)
endif(CUSTOM_AIRSPY_KERNELS)


# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# Finish configuring compiler / linker settings & flags
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${GNURADIO_OSMOSDR_INCLUDE_DIRS}
)

link_directories(
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
    ${ICU4C_LIBRARY_DIRS}
)

# Install desktop
install(FILES dk.gqrx.gqrx.desktop DESTINATION share/applications)

# Install appstream / metainfo file
install(FILES dk.gqrx.gqrx.appdata.xml DESTINATION share/metainfo)

# Install icon
install(FILES resources/icons/gqrx.svg DESTINATION share/icons/hicolor/scalable/apps)

# Add subdirectories
add_subdirectory(src)

# uninstall target
# https://cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
