
if(AVS_INCLUDED)
  return()
endif(AVS_INCLUDED)
set(AVS_INCLUDED true)

if (VICOS)
  set(LIBAVS_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/avs-device-sdk/vicos/include")
  set(LIBAVS_LIB_PATH "${ANKI_EXTERNAL_DIR}/avs-device-sdk/vicos/lib")
elseif (MACOSX)
  set(LIBAVS_INCLUDE_PATH "${ANKI_EXTERNAL_DIR}/avs-device-sdk/mac/include")
  set(LIBAVS_LIB_PATH "${ANKI_EXTERNAL_DIR}/avs-device-sdk/mac/lib")
endif()


set(AVS_LIBS
  ACL
  ADSL
  AFML
  AIP
  Alerts
  AudioPlayer
  AudioResources
  AVSCommon
  AVSSystem
  DeviceSettings
  DoNotDisturbCA
  CapabilitiesDelegate
  CBLAuthDelegate
  CertifiedSender
  ContextManager
  ESP
  InteractionModel
  Notifications
  PlaybackController
  PlaylistParser
  RegistrationManager
  Settings
  SpeakerManager
  SpeechEncoder
  SpeechSynthesizer
  SQLiteStorage
)

if (VICOS)
  set(AVS_LIB_EXT ".so")
elseif (MACOSX)
  set(AVS_LIB_EXT ".dylib")
endif()

foreach(LIB ${AVS_LIBS})
  add_library(${LIB} SHARED IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${LIBAVS_LIB_PATH}/lib${LIB}${AVS_LIB_EXT}"
    INTERFACE_INCLUDE_DIRECTORIES
    "${LIBAVS_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "Apache-2.0,${CMAKE_SOURCE_DIR}/licenses/avs-device-sdk.license" "curl,${CMAKE_SOURCE_DIR}/licenses/curl.license" "OpenSSL-SSLeay,${CMAKE_SOURCE_DIR}/licenses/openssl.license" "MIT,${CMAKE_SOURCE_DIR}/licenses/nghttp2.license")
endforeach()

#only needed for vicos
if (TARGET copy_avs_libs)
    return()
endif()

message(STATUS "avs libs: ${AVS_LIBS}")

set(AVS_LIB_NAMES "")
set(AVS_LIB_PATHS "")

foreach(lib ${AVS_LIBS})
    get_target_property(libimport ${lib} IMPORTED_LOCATION)
    get_filename_component(libname ${libimport} NAME)
    set(libpath "${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/${libname}")
    add_custom_command(
        OUTPUT ${libpath}
        DEPENDS ${libimport}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${libimport} ${libpath}
        COMMENT "install ${libname}"
        VERBATIM
    )
    list(APPEND AVS_LIB_NAMES ${libname})
    list(APPEND AVS_LIB_PATHS ${libpath})
endforeach()

add_custom_target(copy_avs_libs ALL DEPENDS ${AVS_LIB_PATHS})
