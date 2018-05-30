cmake_minimum_required(VERSION 2.8.11)

# Project name
project(gqrx)
set(${PROJECT_NAME}_MAJOR "2")
set(${PROJECT_NAME}_MINOR "11")
set(${PROJECT_NAME}_PATCH "5")
set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}.${${PROJECT_NAME}_PATCH}")
##set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}")
##add_definitions(-DVERSION="${VERSION}")

# development version
execute_process(
    COMMAND git describe --long --dirty
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GITVERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
add_definitions(-DVERSION="${GITVERSION}")

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

# Disable debug outputs in release builds
if(${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    add_definitions(-DQT_NO_DEBUG_OUTPUT)
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    set(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    add_definitions(-Wall)
    add_definitions(-Wextra)
    add_definitions(-Wno-unused-parameter)
    add_definitions(-Wsign-compare)
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


# 3rd Party Dependency Stuff
find_package(Qt5 COMPONENTS Core Network Widgets Svg REQUIRED)
find_package(Boost COMPONENTS system program_options REQUIRED)
set(GR_REQUIRED_COMPONENTS RUNTIME ANALOG AUDIO BLOCKS DIGITAL FILTER FFT PMT)
find_package(Gnuradio REQUIRED)
find_package(Gnuradio-osmosdr REQUIRED)

if(NOT GNURADIO_RUNTIME_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to compile gqrx")
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    if(NOT LINUX_AUDIO_BACKEND)
        set(LINUX_AUDIO_BACKEND Pulseaudio CACHE STRING "Choose the audio backend, options are: Pulseaudio, Portaudio, Gr-audio" FORCE)
    endif()

    if(${LINUX_AUDIO_BACKEND} MATCHES "Pulseaudio")
        find_package(PulseAudio REQUIRED)
        # there is a defect in the pulse audio cmake file that does not include this library. So we add it here.
        find_library(PULSE-SIMPLE NAMES pulse-simple REQUIRED)
        add_definitions(-DWITH_PULSEAUDIO)
        unset(PORTAUDIO_INCLUDE_DIRS CACHE)
        unset(PORTAUDIO_LIBRARIES CACHE)
    elseif(${LINUX_AUDIO_BACKEND} MATCHES "Portaudio")
        find_package(Portaudio REQUIRED)
        add_definitions(-DWITH_PORTAUDIO)
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
    elseif(${LINUX_AUDIO_BACKEND} MATCHES "Gr-audio")
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
        find_package(Portaudio REQUIRED)
        add_definitions(-DWITH_PORTAUDIO)
        unset(PULSEAUDIO_FOUND CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_LIBRARY CACHE)
        unset(PulseAudio_DIR CACHE)
        unset(PULSE-SIMPLE CACHE)
        unset(PULSEAUDIO_INCLUDE_DIR CACHE)
        unset(PULSEAUDIO_MAINLOOP_LIBRARY CACHE)
    elseif(${OSX_AUDIO_BACKEND} MATCHES "Gr-audio")
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
    ${Boost_INCLUDE_DIRS}
    ${GNURADIO_RUNTIME_INCLUDE_DIRS}
    ${GNURADIO_OSMOSDR_INCLUDE_DIRS}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
)


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
