set(WWISE_LIBS
  # Engine
  CommunicationCentral
  AkStreamMgr
  AkMusicEngine
  AkSoundEngine
  AkMemoryMgr
  # Audio Kinetic Effect Plugins
  AkCompressorFX
  AkDelayFX
  AkMatrixReverbFX
  AkMeterFX
  AkExpanderFX
  AkParametricEQFX
  AkGainFX
  AkPeakLimiterFX
  AkSoundSeedImpactFX
  AkRoomVerbFX
  AkGuitarDistortionFX
  AkStereoDelayFX
  AkPitchShifterFX
  AkTimeStretchFX
  AkFlangerFX
  AkConvolutionReverbFX
  AkTremoloFX
  AkHarmonizerFX
  AkRecorderFX
  # McDSP
  McDSPLimiterFX
  McDSPFutzBoxFX
  # Crankcase REV
  # CrankcaseAudioREVModelPlayerFX
  # Sources plug-ins
  AkSilenceSource
  AkSineSource
  AkToneSource
  AkAudioInputSource
  AkMotionGeneratorSource
  AkSoundSeedWooshSource
  AkSoundSeedWindSource
  AkSynthOneSource
  # Required by codecs plug-ins
  AkVorbisDecoder
)

set(WWISE_SDK_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/wwise/versions/current)
set(WWISE_INCLUDE_PATH ${WWISE_SDK_ROOT}/include)

# Set Lib Platform
if(VICOS)
  set(AUDIO_PLATFORM vicos)
  list(APPEND WWISE_LIBS
    iZHybridReverbFX
    iZTrashBoxModelerFX
    iZTrashDelayFX
    iZTrashDistortionFX
    iZTrashDynamicsFX
    iZTrashFiltersFX
    iZTrashMultibandDistortionFX
  )

elseif(MACOSX)
  set(AUDIO_PLATFORM mac)
  list(APPEND WWISE_LIBS AkAACDecoder)

elseif(IOS)
  set(AUDIO_PLATFORM ios)
  list(APPEND WWISE_LIBS AkAACDecoder)

elseif(ANDROID)
  set(AUDIO_PLATFORM android)

else()
  message(FATAL_ERROR "Unknown Audio Platform")
endif()

# Set lib configuration
set(AUDIO_CONFIG "profile")
set(ANKI_AUDIO_DEFINITIONS AK_COMM_NO_DYNAMIC_PORTS=1)

option(AUDIO_RELEASE "Use Wwise release libs and turn profiler off" OFF)
if(AUDIO_RELEASE)
  # Use Wwise release libs
  set(AUDIO_CONFIG "release")
  # Set release flags
  list(APPEND ANKI_AUDIO_DEFINITIONS
    AK_OPTIMIZED=1
  )
endif(AUDIO_RELEASE)

set(WWISE_LIB_PATH ${WWISE_SDK_ROOT}/libs/${AUDIO_PLATFORM}/${AUDIO_CONFIG})
message(STATUS "Wwise lib path: ${WWISE_LIB_PATH}")

# Add Libs
foreach(LIB ${WWISE_LIBS})
  add_library(${LIB} STATIC IMPORTED)
  set_target_properties(${LIB} PROPERTIES
    IMPORTED_LOCATION
    "${WWISE_LIB_PATH}/lib${LIB}.a"
    INTERFACE_INCLUDE_DIRECTORIES
    "${WWISE_INCLUDE_PATH}")
  anki_build_target_license(${LIB} "Commercial")
endforeach()
