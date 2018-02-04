cmake_minimum_required(VERSION 2.8.12)

function(assign_source_group)
    foreach(_source IN ITEMS ${ARGN})
        if (IS_ABSOLUTE "${_source}")
            file(RELATIVE_PATH _source_rel "${CMAKE_CURRENT_SOURCE_DIR}" "${_source}")
        else()
            set(_source_rel "${_source}")
        endif()
        get_filename_component(_source_path "${_source_rel}" PATH)
        string(REPLACE "/" "\\" _source_path_msvc "${_source_path}")
        source_group("${_source_path_msvc}" FILES "${_source}")
    endforeach()
endfunction(assign_source_group)

# ----------------------------------------------------------------------------
project(miniplayer)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(LINK_SUFFIX x64)
else()
    set(LINK_SUFFIX x86)
endif()

if(MSVC_VERSION EQUAL 1800)
    set(MSVC_SUFFIX 2013)
elseif(MSVC_VERSION EQUAL 1900)
    set(MSVC_SUFFIX 2015)
else()
    message(FATAL_ERROR "unknown msvc version ...")
endif()

if(MSVC AND NOT EXISTS "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user")
	file(GENERATE
		OUTPUT "${CMAKE_BINARY_DIR}/ALL_BUILD.vcxproj.user"
		INPUT "${CMAKE_SOURCE_DIR}/cmake/ALL_BUILD.vcxproj.user.in")
endif()

if(CMAKE_CXX_FLAGS MATCHES "/W[0-4]")
    string(REGEX REPLACE "/W[0-4]" "/W4" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
else()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")
endif()

# Disable pointless constant condition warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4127 /wd4201")

if(WIN32)
	add_definitions(-DUNICODE -D_UNICODE -D_CRT_SECURE_NO_WARNINGS)
endif()

set(CMAKE_C_FLAGS_DEBUG "/DDEBUG=1 /D_DEBUG=1 ${CMAKE_C_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_DEBUG "/DDEBUG=1 /D_DEBUG=1 ${CMAKE_C_FLAGS_DEBUG}")

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SAFESEH:NO")
    set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} /SAFESEH:NO")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR TRUE)

set(src_DIR "src")
file(GLOB_RECURSE miniplayer_SRC
    "${src_DIR}/*.h*"
    "${src_DIR}/*.c*"
    "${src_DIR}/miniplayer.rc"
    )
assign_source_group(${miniplayer_SRC})

#include_directories-------------
#ffmpeg
include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/player_deps/ffmpeg_deps/include")
link_directories("${CMAKE_CURRENT_LIST_DIR}/player_deps/ffmpeg_deps/${LINK_SUFFIX}/lib")
#SDL2
include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/player_deps/sdl_deps/include")
link_directories("${CMAKE_CURRENT_LIST_DIR}/player_deps/sdl_deps/${LINK_SUFFIX}/lib")
#DirectX
#include_directories(SYSTEM "${CMAKE_CURRENT_LIST_DIR}/src/DirectX_SDK")



add_executable(miniplayer WIN32
	${miniplayer_SRC}
    )

if(WIN32)
target_link_libraries(miniplayer 
SDL2.lib 
SDL2main.lib)

target_link_libraries(miniplayer
avdevice.lib 
avcodec.lib 
avfilter.lib 
avformat.lib 
avutil.lib 
postproc.lib 
swresample.lib 
swscale.lib)

target_link_libraries(miniplayer
d3d9.lib
xaudio2.lib)
endif()

if(MSVC)
set_target_properties(miniplayer PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
endif()

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLAYER_OUTPUT_SUFFIX 64)
else()
    set(PLAYER_OUTPUT_SUFFIX 32)
endif()

set_target_properties(miniplayer PROPERTIES OUTPUT_NAME "MiniPlayer${PLAYER_OUTPUT_SUFFIX}")

add_custom_command(
    TARGET miniplayer POST_BUILD
    COMMAND "${CMAKE_COMMAND}"
   "-DCONFIG=$<CONFIGURATION>"
    -P "${CMAKE_SOURCE_DIR}/cmake/postbuild_copy_helper.cmake"
    VERBATIM
    )
