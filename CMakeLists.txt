cmake_minimum_required(VERSION 3.7.0)

project(whisper-alsa CXX)

set(CMAKE_CXX_STANDARD 17)

if (NOT WHISPER_CPP_DIR)
    find_path(WHISPER_CPP_DIR "whisper.h" REQUIRED)
endif()

find_package(Boost COMPONENTS system thread log program_options REQUIRED)
include_directories(aes67-daemon ${RAVENNA_ALSA_LKM_DIR}/common ${RAVENNA_ALSA_LKM_DIR}/driver ${CPP_HTTPLIB_DIR} ${Boost_INCLUDE_DIR})
add_definitions( -DBOOST_LOG_DYN_LINK -DBOOST_LOG_USE_NATIVE_SYSLOG )
add_compile_options( -Wall -g )
set(SOURCES  main.cpp log.cpp capture.cpp transcriber.cpp whisper.cpp)

add_executable(whisper-alsa ${SOURCES})

target_link_libraries(whisper-alsa ${Boost_LIBRARIES})

include_directories(whisper-alsa ${WHISPER_CPP_DIR}/include ${WHISPER_CPP_DIR}/ggml/include)
find_library(ALSA_LIBRARY NAMES asound)
find_library(WHISPER_LIBRARY HINTS ${WHISPER_CPP_DIR}/build/src NAMES whisper)
find_library(GGML_BASE_LIBRARY HINTS ${WHISPER_CPP_DIR}/build/ggml/src NAMES ggml-base)
find_library(GGML_LIBRARY HINTS ${WHISPER_CPP_DIR}/build/ggml/src NAMES ggml)
find_library(GGML_CPU_LIBRARY HINTS ${WHISPER_CPP_DIR}/build/ggml/src NAMES ggml-cpu)
target_link_libraries(whisper-alsa ${ALSA_LIBRARY} ${WHISPER_LIBRARY} ${GGML_BASE_LIBRARY} ${GGML_LIBRARY} ${GGML_CPU_LIBRARY})

