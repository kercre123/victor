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
  'text_to_speech_base': [
    '<(coretech_external_path)/flite-2.0.0',
  ],
  'text_to_speech_include_dirs': [
    '<(text_to_speech_base)/include',
  ],
  'text_to_speech_library_dirs': [
  ],
  'text_to_speech_libraries': [
  ],
  
  'conditions': [
    # Platform-specific values
    ['OS=="mac"', {
      'text_to_speech_library_dirs': [
        '<(text_to_speech_base)/generated/mac/DerivedData/Release',
      ],
      'text_to_speech_libraries': [
        'libflite_cmu_grapheme_lang.a',
        'libflite_cmu_grapheme_lex.a',
        'libflite_cmu_us_kal.a',
        'libflite_cmu_us_rms.a',
        'libflite_cmulex.a',
        'libflite_usenglish.a',
        'libflite.a',
      ],
    }],
    
    ['OS=="ios"', {
      'text_to_speech_library_dirs': [
        '<(text_to_speech_base)/generated/ios/DerivedData/Release-iphoneos',
      ],
      'text_to_speech_libraries': [
        'libflite_cmu_grapheme_lang.a',
        'libflite_cmu_grapheme_lex.a',
        'libflite_cmu_us_kal.a',
        'libflite_cmu_us_rms.a',
        'libflite_cmulex.a',
        'libflite_usenglish.a',
        'libflite.a',
      ],
    }],
    
    ['OS=="android"', {
      # Use explicit path to each library because android
      'text_to_speech_libraries': [
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_cmu_grapheme_lang.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_cmu_grapheme_lex.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_cmu_us_kal.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_cmu_us_rms.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_cmulex.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite_usenglish.a',
        '<(text_to_speech_base)/build/armeabiv7a-android/lib/libflite.a',
      ],
    }],

  ], # conditions
  
}, # variables

} # gyp
