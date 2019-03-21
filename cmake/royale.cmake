if(MACOSX)
  SET(ROYALE_LIB_DIR "${CMAKE_SOURCE_DIR}/lib/royale/bin")
  SET(ROYALE_INCLUDE_DIR "${CMAKE_SOURCE_DIR}/lib/royale/include")

  set (ROYALE_LIBS royale)

  foreach(LIB ${ROYALE_LIBS})
    add_library(${LIB} SHARED IMPORTED)
    set_target_properties(${LIB} PROPERTIES 
    IMPORTED_LOCATION
    "${ROYALE_LIB_DIR}/lib${LIB}.dylib"
    INTERFACE_INCLUDE_DIRECTORIES
    "${ROYALE_INCLUDE_DIR}")
    anki_build_target_license(${LIB} "Commercial")
  endforeach()
endif()
