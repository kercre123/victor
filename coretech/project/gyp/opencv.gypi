{
  'variables' : {
  
    'opencv_includes': [
      # '<(coretech_external_path)/opencv-2.4.8/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/core/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/highgui/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/imgproc/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/contrib/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/calib3d/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/objdetect/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/video/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/features2d/include',
      '<(coretech_external_path)/opencv-2.4.8/modules/flann/include',
    ],
    
    'opencv_libs': [
      'libzlib.a',
      'liblibjpeg.a',
      'liblibpng.a',
      'liblibtiff.a',
      'liblibjasper.a',
      'libIlmImf.a',
      'libopencv_core.a',
      'libopencv_imgproc.a',
      'libopencv_highgui.a',
      'libopencv_calib3d.a',
      'libopencv_contrib.a',
      'libopencv_objdetect.a',
      'libopencv_video.a',
      'libopencv_features2d.a',
    ],
    
  }, # variables
  
  'target_defaults': {
    'configurations': {
      'Debug': {
        'conditions': [
          ['OS=="ios"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-ios/multiArchLibs',
              ],
            },
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-2.4.8/lib/Debug',
                '<(coretech_external_path)/build/opencv-2.4.8/3rdparty/lib/Debug',
              ],
            },
          }],
        ],
      },
      'Profile': {
        'conditions': [
          ['OS=="ios"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-ios/multiArchLibs',              
              ],
            },
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-2.4.8/lib/Release',
                '<(coretech_external_path)/build/opencv-2.4.8/3rdparty/lib/Release',
              ],
            },
          }],
        ],
      },
      'Release': {
        'conditions': [
          ['OS=="ios"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-ios/multiArchLibs',
              ],
            },
          }],
          ['OS=="mac"', {
            'xcode_settings': {
              'LIBRARY_SEARCH_PATHS': [
                '<(coretech_external_path)/build/opencv-2.4.8/lib/Release',
                '<(coretech_external_path)/build/opencv-2.4.8/3rdparty/lib/Release',
              ],
            },
          }],
        ],
      },
    }, # configurations  
  } # target_defaults
  
}