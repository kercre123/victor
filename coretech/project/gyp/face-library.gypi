{
'variables': {

  # Here we pick which face library to use and set its path/includes/libs
  # initially to empty. They will be filled in below in the 'conditions' as
  # needed:
  # (NOTE: This is duplicated from coretech-internal.gyp!)
  'face_library': 'facesdk', # one of: 'opencv', 'faciometric', or 'facesdk'
  'face_library_path':         [ ],
  'face_library_includes':     [ ],
  'face_library_libs':         [ ],
  'face_library_lib_path':     [ ],
      
  'conditions': [
    # Face libraries, depending on platform:
    
    # Faciometric:
    ['face_library=="faciometric"', {
      'face_library_path': [
        '<(coretech_external_path)/IntraFace_126_FaceRecog_OSX_3weeks',
      ],
    }],
  
    ['OS=="mac" and face_library=="faciometric"', {
      'face_library_includes': [
        '<(face_library_path)/Demo/include',
        '<(face_library_path)/Demo/3rdparty/Eigen3/include',
      ],
      'face_library_lib_path': [
        '<(face_library_path)/Demo/lib',
      ],
      'face_library_libs': [
        'libintraface_core126.dylib',
        'libintraface_frecog.dylib',
        #'libintraface_emo126.dylib',
        #'libintraface_gaze126.dylib',
      ],
    }],
    
    ['OS=="ios" and face_library=="faciometric"', {
      'face_library_includes': [
        '<(face_library_path)/IntraFace_126_iOS_Anki/Library/3rdparty/include',
      ],
      'face_library_lib_path': [
        '<(face_library_path)/IntraFace_126_iOS_Anki/Library',
      ],
      'face_library_libs': [
        'intraface.framework',
      ],
    }],
    
    # FaceSDK:
    ['face_library=="facesdk"', {
      'face_library_includes': [
        '<(coretech_external_path)/Luxand_FaceSDK/include/C',
      ],
    }],
      
    ['OS=="mac" and face_library=="facesdk"', {
      'face_library_libs': [
        'libfsdk.dylib',
      ],
      'face_library_lib_path': [
        '<(coretech_external_path)/Luxand_FaceSDK/bin/osx_x86_64',
      ]
    }],
    
    ['OS=="ios" and face_library=="facesdk"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/Luxand_FaceSDK/bin/iOS',
      ],
      'face_library_libs': [
        # TODO: handle different architectures
        'libfsdk-static.a',
      ],
    }],
    
    ['OS=="android" and face_library=="facesdk"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/Luxand_FaceSDK/bin/Android/armeabi-v7a',
      ],
      'face_library_libs': [
        # TODO: handle different architectures
        'libfsdk.so',
        'libstlport_shared.so',
      ],
    }],
  ], # conditions
  
}, # variables

'target_defaults': {
  'xcode_settings': {
    'conditions': [
      ['OS=="ios" and face_library=="faciometric"', {
         'FRAMEWORK_SEARCH_PATHS': '<(face_library_path)/IntraFace_126_iOS_Anki/Library',
      }],
    ],
  },
  
  'configurations': {
    'Debug': {
      'xcode_settings': {
        'LIBRARY_SEARCH_PATHS': ['<(face_library_lib_path)'],
      },
    },
    'Profile': {
      'xcode_settings': {
        'LIBRARY_SEARCH_PATHS': ['<(face_library_lib_path)'],
      },
    },
    'Release': {
      'xcode_settings': {
        'LIBRARY_SEARCH_PATHS': ['<(face_library_lib_path)'],
      },
    },
  }, # configurations
  
} # target_defaults

}