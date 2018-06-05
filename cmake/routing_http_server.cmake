set(ROUTING_HTTP_INCLUDE_PATH "${CORETECH_EXTERNAL_DIR}/routing_http_server/generated/include")

# routing_http_server is only available on ios, mac
set(ROUTING_HTTP_LIBS "")
set(PLATFORM_ROUTING_HTTP_LIBS "")

set(ROUTING_HTTP_LIB_PATH "")

if (MACOSX)
  list(APPEND ROUTING_HTTP_LIBS routing_http_server)
  set(ROUTING_HTTP_LIB_PATH
    "${CORETECH_EXTERNAL_DIR}/routing_http_server/generated/mac/DerivedData/Release")

  find_library(SECURITY Security)
  list(APPEND PLATFORM_ROUTING_HTTP_LIBS ${SECURITY})
endif()

foreach(LIB ${ROUTING_HTTP_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${ROUTING_HTTP_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${ROUTING_HTTP_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "BSD-2")
endforeach()

list(APPEND ROUTING_HTTP_LIBS ${PLATFORM_ROUTING_HTTP_LIBS})
