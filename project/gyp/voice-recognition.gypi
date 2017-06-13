{
'variables': {

  # Here we set the recognition library path/includes/libs
  # initially to empty. They will be filled in below in the 'conditions' as
  # needed:
  'voice_recog_library_path':         [ ],
  'voice_recog_library_includes':     [ ],
  'voice_recog_library_libs':         [ ],
  'voice_recog_library_lib_path':     [ ],
      
  'conditions': [
    # Variable definitions, depending on platform:

    ['OS=="mac"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/x86_64-darwin/lib',
      ],
      'voice_recog_library_libs': [
        'libthf.a',
      ],
      'voice_recog_library_includes': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23/x86_64-darwin/include',
      ]
    }],
    
    ['OS=="ios"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/ios/lib',
      ],
      'voice_recog_library_libs': [
        'libthf.a',
      ],
      'voice_recog_library_includes': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23/ios/include',
      ]
    }],
    
    ['OS=="android"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/android/lib'
      ],
      'voice_recog_library_libs': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23_noexpire/android/lib/libthf_armeabi-v7a.a' # android special case wants full path of lib
      ],
      'voice_recog_library_includes': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.23/android/include',
      ]
    }],
  ], # conditions
}, # variables
}
