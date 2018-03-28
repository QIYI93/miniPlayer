file(GLOB FFMPEG_DEB
    "${CMAKE_SOURCE_DIR}/player_deps/ffmpeg_deps/${LINK_SUFFIX}/bin/*.dll"
    )
file(GLOB FFMPEG_REL
    "${CMAKE_SOURCE_DIR}/player_deps/ffmpeg_deps/${LINK_SUFFIX}/bin/*.dll"
    )
	
file(GLOB SDL_DEB
    "${CMAKE_SOURCE_DIR}/player_deps/sdl_deps/${LINK_SUFFIX}/bin/*.dll"
    )
file(GLOB SDL_REL
    "${CMAKE_SOURCE_DIR}/player_deps/sdl_deps/${LINK_SUFFIX}/bin/*.dll"
    )

foreach(
    list
    FFMPEG_DEB
	FFMPEG_REL
	SDL_DEB
	SDL_REL
    )
	if(${list})
		list(REMOVE_DUPLICATES ${list})
	endif()
endforeach()

message(STATUS "FFMPEG_DEB             : ${FFMPEG_DEB}")
message(STATUS "FFMPEG_REL             : ${FFMPEG_REL}")
message(STATUS "SDL_DEB                : ${SDL_DEB}")
message(STATUS "SDL_REL                : ${SDL_REL}")


function(copy_into_d filelist)
    copy_into("${filelist}" "${CMAKE_BINARY_DIR}/deps/${LINK_SUFFIX}/d/")
endfunction(copy_into_d)

function(copy_into_r filelist)
    copy_into("${filelist}" "${CMAKE_BINARY_DIR}/deps/${LINK_SUFFIX}/r/")
endfunction(copy_into_r)

function(copy_into_d_subdir subdir filelist)
    copy_into("${filelist}" "${CMAKE_BINARY_DIR}/deps/${LINK_SUFFIX}/d/${subdir}/")
endfunction(copy_into_d_subdir)

function(copy_into_r_subdir subdir filelist)
    copy_into("${filelist}" "${CMAKE_BINARY_DIR}/deps/${LINK_SUFFIX}/r/${subdir}/")
endfunction(copy_into_r_subdir)

function(copy_into filelist dir)
    foreach (f ${filelist})
        message(STATUS "copying ${f} to ${dir}")
        file(COPY "${f}" DESTINATION "${dir}")
    endforeach()
endfunction(copy_into)

file(REMOVE_RECURSE "${CMAKE_BINARY_DIR}/deps/")

copy_into_d("${FFMPEG_DEB}")
copy_into_r("${FFMPEG_REL}")
copy_into_d("${SDL_DEB}")
copy_into_r("${SDL_REL}")
