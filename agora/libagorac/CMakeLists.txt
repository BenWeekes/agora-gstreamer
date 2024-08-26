cmake_minimum_required(VERSION 3.10)

project(gstagorac)

# require a C++11 compiler and use it for all targets
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -fPIC -g")


set(AGORA_SDK_DIR $ENV{AGORA_SDK_DIR})
Message("Path to agora sdk: ${AGORA_SDK_DIR}" )
include_directories(${AGORA_SDK_DIR}/agora_sdk/include)
include_directories(${CMAKE_SOURCE_DIR})

#useful for installing in a custom path
set(INSTALL_PATH $ENV{INSTALL_PATH})
if(NOT INSTALL_PATH)
  message("install directory not set, will use  /usr/local/ to install")
  set (INSTALL_PATH /usr/local)
endif()

set(HELPER_FILES helpers/agoradecoder.cpp 
              helpers/agoraencoder.cpp 
	      helpers/agoralog.cpp 
	      helpers/localconfig.cpp 
	      helpers/utilities.cpp
	      helpers/uidtofile.cpp
	      syncbuffer.cpp)

set(AGORA_OBSERCER  observer/pcmframeobserver.cpp 
		    observer/h264frameobserver.cpp 
		    observer/connectionobserver.cpp 
		    observer/userobserver.cpp)

set(AGORA_HELPERS agoraevent.cpp)
set (AGORA_IO_FILES agoraio.cpp)

set(LIB_SOURCE ${HELPER_FILES} 
               ${AGORA_OBSERCER} 
               ${AGORA_HELPERS} 
               ${AGORA_IO_FILES}
               agorac.cpp)

add_library(gstagorac SHARED ${LIB_SOURCE})


install(TARGETS gstagorac DESTINATION ${INSTALL_PATH}/lib)

#install agora sdk
add_custom_command(
        TARGET gstagorac POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${AGORA_SDK_DIR}/agora_sdk/libagora_rtc_sdk.so
                ${INSTALL_PATH}/lib)

	#add_custom_command(
	# TARGET gstagorac POST_BUILD
	# COMMAND ${CMAKE_COMMAND} -E copy
	#        ${AGORA_SDK_DIR}/agora_sdk/libagora-ffmpeg.so
	#        ${INSTALL_PATH}/lib)

#install agoralib headers
add_custom_command(
        TARGET gstagorac POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/agorac.h 
                ${INSTALL_PATH}/include)

add_custom_command(
        TARGET gstagorac POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
                ${CMAKE_SOURCE_DIR}/agoraconfig.h 
                ${INSTALL_PATH}/include)
