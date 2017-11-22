
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
	set(BIN_SUFFIX x64)
else()
	set(BIN_SUFFIX x86)
endif()

if(${CONFIG} STREQUAL Debug)
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/ffmpeg_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/sdl_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")
elseif(${CONFIG} STREQUAL RelWithDebInfo)
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/ffmpeg_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/sdl_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")
else()
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/ffmpeg_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")
    file(COPY "${CMAKE_BINARY_DIR}/../player_deps/sdl_deps/${BIN_SUFFIX}/bin/" DESTINATION "${CMAKE_BINARY_DIR}/${CONFIG}/")

endif()
