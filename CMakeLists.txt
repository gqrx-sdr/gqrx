CMAKE_MINIMUM_REQUIRED(VERSION 2.8.0)

#######################################################################################################################
# Project name
project(gqrx)
set(${PROJECT_NAME}_MAJOR "2")
set(${PROJECT_NAME}_MINOR "4")
#set(${PROJECT_NAME}_PATCH "0")
#set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}.${${PROJECT_NAME}_PATCH}")
set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}")
set(PACKAGE ${PROJECT_NAME})
add_definitions(-DVERSION="${VERSION}")

# development version
#execute_process(
#    COMMAND git describe --long --dirty
#    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
#    OUTPUT_VARIABLE GITVERSION
#    OUTPUT_STRIP_TRAILING_WHITESPACE
#)
#add_definitions(-DVERSION="${GITVERSION}")

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
IF(${CMAKE_BUILD_TYPE} MATCHES "Valgrind")
    SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
ENDIF()

# Disable debug outputs in release builds
IF(${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    ADD_DEFINITIONS(-DQT_NO_DEBUG_OUTPUT)
ENDIF()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # using regular Clang or AppleClang
    SET(CMAKE_COMPILER_IS_CLANGXX 1)
endif()

IF(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_COMPILER_IS_CLANGXX)
    ADD_DEFINITIONS(-Wall)
    ADD_DEFINITIONS(-Wextra)
    ADD_DEFINITIONS(-Wno-unused-parameter)
    ADD_DEFINITIONS(-Wsign-compare)
ENDIF()

#######################################################################################################################
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


#######################################################################################################################
# 3rd Party Dependency Stuff
find_package(Qt5 COMPONENTS Core Network Widgets REQUIRED)
find_package(Boost COMPONENTS system program_options REQUIRED)
set(GR_REQUIRED_COMPONENTS RUNTIME ANALOG AUDIO BLOCKS DIGITAL FILTER FFT PMT)
find_package(Gnuradio REQUIRED)
find_package(Gnuradio-osmosdr REQUIRED)

if(NOT GNURADIO_RUNTIME_FOUND)
    message(FATAL_ERROR "GnuRadio Runtime required to compile gr-air-modes")
endif()

IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
    find_package(PulseAudio REQUIRED)
    # there is a defect in the pulse audio cmake file that does not include this library. So we add it here.
    find_library(PULSE-SIMPLE NAMES pulse-simple REQUIRED)
    ADD_DEFINITIONS(-DWITH_PULSEAUDIO)
ENDIF()

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#######################################################################################################################
# Finish configuring compiler / linker settings & flags
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${Boost_INCLUDE_DIRS}
    ${GNURADIO_RUNTIME_INCLUDE_DIRS}
)

link_directories(
    ${Boost_LIBRARY_DIRS}
    ${GNURADIO_RUNTIME_LIBRARY_DIRS}
)



#######################################################################################################################
# Add subdirectories
add_subdirectory(src)

#######################################################################################################################
# enable testing - must be in the top level CMakeLists.txt file
# enable_testing()


# uninstall target
# https://cmake.org/Wiki/CMake_FAQ#Can_I_do_.22make_uninstall.22_with_CMake.3F
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
)
