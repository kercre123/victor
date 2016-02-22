{
  'variables' : {
    'opencv_version': '3.1.0',
    
    'opencv_includes': [
      # '<(coretech_external_path)/opencv-<(opencv_version)/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/core/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/highgui/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/imgproc/include',
      #'<(coretech_external_path)/opencv-<(opencv_version)/modules/contrib/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/calib3d/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/objdetect/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/video/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/features2d/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/flann/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/imgcodecs/include',
      '<(coretech_external_path)/opencv-<(opencv_version)/modules/videoio/include',
    ],
    
    'conditions': [
      [
        'OS=="ios"',
        {
          'opencv_libs': [
            'opencv2.framework'
          ],
          
          'opencv_lib_search_path_debug': [
            '<(coretech_external_path)/build/opencv-ios',
          ],
          
          'opencv_lib_search_path_release': [
            '<(coretech_external_path)/build/opencv-ios',
          ],
        },
        
        'OS=="mac"',
        {
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
            #'libopencv_contrib.a',
            'libopencv_objdetect.a',
            'libopencv_video.a',
            'libopencv_features2d.a',
            'libopencv_imgcodecs.a',
            'libopencv_videoio.a',
          ],
          
          'opencv_lib_search_path_debug': [
            '<(coretech_external_path)/build/opencv-<(opencv_version)/lib/Debug',
            '<(coretech_external_path)/build/opencv-<(opencv_version)/3rdparty/lib/Debug',
          ],
          
          'opencv_lib_search_path_release': [
            '<(coretech_external_path)/build/opencv-<(opencv_version)/lib/Release',
            '<(coretech_external_path)/build/opencv-<(opencv_version)/3rdparty/lib/Release',
          ],
        },
      ],
    ],
    
  }, # variables
  
}

