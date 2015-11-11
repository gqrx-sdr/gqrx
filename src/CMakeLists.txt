#######################################################################################################################
# Add the source subdirectories
add_subdirectory(applications)
add_subdirectory(dsp)
add_subdirectory(interfaces)
add_subdirectory(qtgui)
add_subdirectory(receivers)

IF(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	add_subdirectory(osxaudio)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
	add_subdirectory(pulseaudio)
ELSEIF(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
ENDIF()

#######################################################################################################################
# bring in the global properties
get_property(${PROJECT_NAME}_SOURCE GLOBAL PROPERTY SRCS_LIST)
get_property(${PROJECT_NAME}_UI_SOURCE GLOBAL PROPERTY UI_SRCS_LIST)


#######################################################################################################################
# Process the UI files
QT5_WRAP_UI(UIS_HDRS
	${${PROJECT_NAME}_UI_SOURCE}
)

#######################################################################################################################
# Process the resources
QT5_ADD_RESOURCES(RESOURCES_LIST
	../resources/icons.qrc
	../resources/textfiles.qrc
)

#######################################################################################################################
# Build the program
add_executable(${PROJECT_NAME} ${${PROJECT_NAME}_SOURCE} ${UIS_HDRS} ${RESOURCES_LIST})
qt5_use_modules(${PROJECT_NAME} Core Network Widgets)
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 11)
# The pulse libraries are only needed on Linux.  On other platforms they will not be found, so having them here is fine.
target_link_libraries(${PROJECT_NAME} 
    ${Boost_LIBRARIES} 
    ${GNURADIO_ALL_LIBRARIES} 
    ${GNURADIO_OSMOSDR_LIBRARIES} 
    ${PULSEAUDIO_LIBRARY}
    ${PULSE-SIMPLE})
