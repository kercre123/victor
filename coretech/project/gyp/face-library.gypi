{
'variables': {

  # Here we pick which face library to use and set its path/includes/libs
  # initially to empty. They will be filled in below in the 'conditions' as
  # needed:
  # (NOTE: This is duplicated from coretech-internal.gyp!)
  'face_library': 'okao', # one of: 'opencv', 'faciometric', 'facesdk', or 'okao'
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

    ['OS=="linux" and face_library=="facesdk"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/Luxand_FaceSDK/bin/Android/armeabi-v7a',
      ],
      'face_library_libs': [
        # TODO: handle different architectures
        'libfsdk.so',
        'libstlport_shared.so',
      ],
    }],
    
    # OKAO Vision:
    ['face_library=="okao"', {
      'face_library_includes': [
        '<(coretech_external_path)/okaoVision/include',
      ],
      'face_library_libs': [
        'libeOkao.a',      # Common
        'libeOkaoCo.a',    # Okao Common
        'libeOkaoDt.a',    # Face Detection
        'libeOkaoPt.a',    # Face Parts Detection
        'libeOkaoEx.a',    # Facial Expression estimation
        'libeOkaoFr.a',    # Face Recognition
        'libeOmcvPd.a',    # Pet Detection
        'libeOkaoPc.a',    # Property Estimation (for Smile/Gaze/Blink?)
        'libeOkaoSm.a',    # Smile Estimation
        'libeOkaoGb.a',    # Gaze & Blink Estimation
      ],
    }],
    
    ['OS=="mac" and face_library=="okao"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/okaoVision/lib/MacOSX',
      ],
    }],
    ['OS=="ios" and face_library=="okao"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/okaoVision/lib/iOS',
      ],
    }],
    ['OS=="android" and face_library=="okao"', {
      'face_library_lib_path': [
        '<(coretech_external_path)/okaoVision/lib/Android/armeabi-v7a',
      ],
    }],
    
  ], # conditions
  
}, # variables

'target_defaults': {
  'defines' : [
    'FACE_TRACKER_FACIOMETRIC=0',
    'FACE_TRACKER_FACESDK=1',
    'FACE_TRACKER_OPENCV=2',
    'FACE_TRACKER_OKAO=3',
    'FACE_TRACKER_TEST=4',
  ],
  'conditions': [
    ['face_library=="faciometric"', {
      'defines' : ['FACE_TRACKER_PROVIDER=FACE_TRACKER_FACIOMETRIC'],
    }],
    ['face_library=="facesdk"', {
      'defines' : ['FACE_TRACKER_PROVIDER=FACE_TRACKER_FACESDK'],
    }],
    ['face_library=="opencv"', {
      'defines' : ['FACE_TRACKER_PROVIDER=FACE_TRACKER_OPENCV'],
    }],
    ['face_library=="okao"', {
      'defines' : ['FACE_TRACKER_PROVIDER=FACE_TRACKER_OKAO'],
    }],
    ['face_library=="test"', {
      'defines' : ['FACE_TRACKER_PROVIDER=FACE_TRACKER_TEST'],
    }],
  ],
  'xcode_settings': {
    'conditions': [
      ['OS=="ios" and face_library=="faciometric"', {
         'FRAMEWORK_SEARCH_PATHS': '<(face_library_path)/IntraFace_126_iOS_Anki/Library',
      }],
    ],
  },

} # target_defaults

}
