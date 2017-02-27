{
'variables': {

  # Here we pick which library to use and set its path/includes/libs
  # initially to empty. They will be filled in below in the 'conditions' as
  # needed:
  'voice_recog_library': 'none', # one of: 'none' or 'thf'
  'voice_recog_library_path':         [ ],
  'voice_recog_library_includes':     [ ],
  'voice_recog_library_libs':         [ ],
  'voice_recog_library_lib_path':     [ ],
      
  'conditions': [
    # Variable definitions, depending on platform:

    # THF:
    ['voice_recog_library=="thf"', {
      'voice_recog_library_includes': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/ios/include',
      ]
    }],
      
    ['OS=="mac" and voice_recog_library=="thf"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/x86_64-darwin/lib',
      ],
      'voice_recog_library_libs': [
        'libthf.a',
      ]
    }],
    
    ['OS=="ios" and voice_recog_library=="thf"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/ios/lib',
      ],
      'voice_recog_library_libs': [
        'libthf.a',
      ]
    }],
    
    ['OS=="android" and voice_recog_library=="thf"', {
      'voice_recog_library_lib_path': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/android/lib'
      ],
      'voice_recog_library_libs': [
        '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/android/lib/libthf_armeabi-v7a.a' # android special case wants full path of lib
      ]
    }],

#['OS=="linux" and voice_recog_library=="thf"', {
#     'voice_recog_library_lib_path': [
#   '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/android/lib',
# ],
#'voice_recog_library_libs': [
        # TODO: handle different architectures
# '<(coretech_external_path)/sensory/TrulyHandsfreeSDK/4.4.4/android/lib/libthf_armeabi-v7a.a' # android special case wants full path of lib
#      ],
# }],
    
  ], # conditions
  
}, # variables

'target_defaults': {
  'defines' : [
    'VOICE_RECOG_NONE=0',
    'VOICE_RECOG_THF=1',
  ],
  'conditions': [
    ['voice_recog_library=="none"', {
      'defines' : ['VOICE_RECOG_PROVIDER=VOICE_RECOG_NONE'],
    }],
    ['voice_recog_library=="thf"', {
      'defines' : ['VOICE_RECOG_PROVIDER=VOICE_RECOG_THF'],
    }],
  ],

} # target_defaults

}
