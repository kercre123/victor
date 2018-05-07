project(ble_cozmo)

set(PLATFORM_SRCS "")
set(PLATFORM_LIBS "")
set(PLATFORM_INCLUDES "")
set(PLATFORM_FLAGS "")
set(PLATFORM_COMPILE_DEFS "")
if (MACOSX)
  find_library(FOUNDATION Foundation)
  find_library(SECURITY Security)
  find_library(CB CoreBluetooth)

  set(PLATFORM_LIBS
    ${FOUNDATION}
    ${SECURITY}
    ${CB}
  )

  set(PLATFORM_FLAGS
    -fobjc-arc
    -Wno-unguarded-availability
  )

  set(PLATFORM_COMPILE_DEFS
    HAVE_GET_RANDOM_BYTES
  )

  set(PLATFORM_INCLUDES
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ios>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ios/Central>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ios/Logging>
  )

  set(PLATFORM_SRCS
    ios/BLECozmoRandom.m
    ios/BLECozmoConnection.h
    ios/BLECozmoConnection.mm
    ios/BLECozmoManager.h
    ios/BLECozmoManager.mm
    ios/BLECozmoRandom.m
    ios/BLECozmoServiceDelegate.h
    ios/Central/BLEAdvertisement.h
    ios/Central/BLEAdvertisement.mm
    ios/Central/BLECentralMultiplexer.h
    ios/Central/BLECentralMultiplexer.mm
    ios/Central/BLECentralServiceDescription.h
    ios/Central/BLECentralServiceDescription.m
    ios/Central/BLEPeripheralConnectionAttempt.h
    ios/Central/BLEPeripheralConnectionAttempt.m
    ios/Logging/BLELog.h
  )

endif()


add_library(ble_cozmo
  shared/BLECozmoMessage.cpp
  ${PLATFORM_SRCS}
)

include(anki_build_strip)
anki_build_strip(TARGET ble_cozmo)

target_link_libraries(ble_cozmo
  PUBLIC
  ${PLATFORM_LIBS}
)

target_compile_options(ble_cozmo
  PRIVATE
  ${PLATFORM_FLAGS}
)

target_compile_definitions(ble_cozmo
  PRIVATE
  ${PLATFORM_COMPILE_DEFS}
)

target_include_directories(ble_cozmo PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/shared>
  "${PLATFORM_INCLUDES}"
)
