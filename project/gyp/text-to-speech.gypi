#
# cozmo-one/project/gyp/text-to-speech.gypi
#
# GYP declarations to link with 3rd party text-to-speech provider
#
{
'variables': {
  #
  # Default values defined for all platforms.
  # Platform-specific values are defined in 'conditions' below.
  #
  'acapela_base': [
    '<(externals_path)/anki-thirdparty/acapela',
  ],
  # Base directory
  'text_to_speech_base': [
  ],
  # Header directories 
  'text_to_speech_include_dirs': [
  ],
  # Library directories
  'text_to_speech_library_dirs': [
  ],
  # Libraries to be linked
  'text_to_speech_libraries': [
  ],
  # Path to Acapela dynamic library (OSX only)
  'text_to_speech_resources_tts': [
  ],
  # Path to Acapela voice data (iOS and Android)
  'text_to_speech_resources_tts_voices': [
  ],
  
  'conditions': [
    # Platform-specific values
    ['OS=="mac"', {
      # Dynamic linkage with libacatts.dylib
      # Dynamic library must be available in search path at runtime
      'text_to_speech_base': [
        '<(acapela_base)/AcapelaTTS_for_Mac_V9.450',
      ],
      'text_to_speech_include_dirs': [
        '<(acapela_base)/AcapelaTTS_for_Mac_V9.450/SDK/Include',
      ],
      'text_to_speech_resources_tts': [
        '<(acapela_base)/AcapelaTTS_for_Mac_V9.450',
      ],
      'text_to_speech_resources_tts_voices': [
        '<(acapela_base)/AcapelaTTS_for_Mac_V9.450/Voices',
      ],
    }],
    
    ['OS=="ios"', {
      # Static linkage with libacattsios.a
      'text_to_speech_base': [
        '<(acapela_base)/AcapelaTTS_for_iOS_V1.851',
      ],
      'text_to_speech_include_dirs': [
        '<(acapela_base)/AcapelaTTS_for_iOS_V1.851/api',
        '<(acapela_base)/AcapelaTTS_for_iOS_V1.851/license',
      ],
      'text_to_speech_library_dirs': [
        '<(acapela_base)/AcapelaTTS_for_iOS_V1.851/libraries/device',
      ],
      'text_to_speech_libraries': [
        'libacattsios.a'
      ],
      'text_to_speech_resources_tts_voices': [
        '<(acapela_base)/Voices_for_Android_iOS',
      ],
    }],
    
    ['OS=="android"', {
      # BUCK linkage with acattsandroid-sdk-library.jar
      # Dynamic linkage with libacattsandroid.so
      'text_to_speech_base': [
        '<(acapela_base)/AcapelaTTS_for_Android_V1.612',
      ],
      'text_to_speech_include_dirs': [
      ],
      'text_to_speech_libraries': [
        '<(acapela_base)/AcapelaTTS_for_Android_V1.612/sdk/armeabi-v7a/libacattsandroid.so',
      ],
      'text_to_speech_resources_tts_voices': [
        '<(acapela_base)/Voices_for_Android_iOS',
      ],
    }],

  ], # conditions
  
}, # variables

} # gyp
