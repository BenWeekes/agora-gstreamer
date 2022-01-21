cmake_minimum_required(VERSION 3.5)
project(AgoraSDKTestStats)

#required libs
add_definitions("-DTARGET_OS_LINUX")
set(lib_sdk "agora_rtc_sdk")
set(lib_ffmpeg "agora-ffmpeg")
set(lib_thread "pthread")

#include dirs
include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_SOURCE_DIR}/agorasdk36/include)

#link dirs
link_directories(${AGORA_MEDIA_SDK_DIR} ${CMAKE_SOURCE_DIR}/agorasdk36/})
link_libraries(${lib_sdk} ${lib_ffmpeg} ${lib_thread})

# Build sample_receive_h264_pcm
file(GLOB SAMPLE_RECEIVE_H264_PCM_CPP_FILES
     test_remote_stats.cpp
     "common/*.cpp",
     common/file_parser/helper_h264_parser.cpp)

add_executable(test-remote-stats ${SAMPLE_RECEIVE_H264_PCM_CPP_FILES})


# Build sample_receive_yuv_pcm
file(GLOB SAMPLE_RECEIVE_YUV_PCM_CPP_FILES
     test_remote_stats_yuv.cpp
     "common/*.cpp",
     common/file_parser/helper_h264_parser.cpp)
add_executable(test-remote-stats-yuv ${SAMPLE_RECEIVE_YUV_PCM_CPP_FILES})




