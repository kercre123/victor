set(NNG_INCLUDE_PATH "${CMAKE_SOURCE_DIR}/lib/nng/vicos/include")

set(NNG_LIBS
  nng
)

set(NNG_LIB_PATH "${CMAKE_SOURCE_DIR}/lib/nng/vicos/lib")

foreach(LIB ${NNG_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${NNG_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${NNG_INCLUDE_PATH}"
    )  
  anki_build_target_license(${LIB} "Apache-2.0,${CMAKE_SOURCE_DIR}/licenses/nng.license")
endforeach()