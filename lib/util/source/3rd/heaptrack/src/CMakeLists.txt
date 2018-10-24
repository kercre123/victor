include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
)

add_subdirectory(util)
add_subdirectory(track)
if (MACOSX)
add_subdirectory(interpret)
add_subdirectory(analyze)
endif()
