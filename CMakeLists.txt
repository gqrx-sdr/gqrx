CMAKE_MINIMUM_REQUIRED(VERSION 3.2.0)

#######################################################################################################################
# Project name
project(gqrx)
set(${PROJECT_NAME}_MAJOR "2")
set(${PROJECT_NAME}_MINOR "3")
set(${PROJECT_NAME}_PATCH "0")
set(VERSION "${${PROJECT_NAME}_MAJOR}.${${PROJECT_NAME}_MINOR}.${${PROJECT_NAME}_PATCH}")
set(PACKAGE ${PROJECT_NAME})

add_definitions(-DVERSION="${VERSION}")

########### Main global variables ###########
# if(NOT CMAKE_BUILD_TYPE)
# 	set(CMAKE_BUILD_TYPE Debug CACHE STRING "Choose the type of build, options are: Debug GProf Valgrind Release" FORCE)
# endif()
#
# set(BUILDTYPE ${CMAKE_BUILD_TYPE})
# string(TOUPPER ${BUILDTYPE} BUILDTYPE)
# add_definitions(-D${BUILDTYPE})

# We have some custom .cmake scripts not in the official distribution.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Add valgrind build options if necessary
IF(${CMAKE_BUILD_TYPE} MATCHES "Valgrind")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")
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
set(GR_REQUIRED_COMPONENTS ANALOG AUDIO BLOCKS DIGITAL FILTER FFT)
find_package(Gnuradio REQUIRED)
find_package(Gnuradio-osmosdr REQUIRED)
IF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	find_package(PulseAudio REQUIRED)
        # there is a defect in the pulse audio cmake file that does not include this library.
        find_library(PULSE-SIMPLE NAMES pulse-simple REQUIRED)
ENDIF()

# Tell CMake to run moc when necessary:
set(CMAKE_AUTOMOC ON)
# As moc files are generated in the binary dir, tell CMake to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#######################################################################################################################
# Finish configuring compiler / linker settings & flags


#######################################################################################################################
# Add subdirectories
add_subdirectory(src)

#######################################################################################################################
# enable testing - must be in the top level CMakeLists.txt file
# enable_testing()

