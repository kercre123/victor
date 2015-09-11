{
  'includes' : [
    '../../coretech/project/gyp/opencv.gypi',
    '../../coretech/project/gyp/face-library.gypi',
  ],

  'variables': {

    # TODO: should this be passed in, or shared?
    'coretech_defines': [
      'ANKICORETECH_USE_MATLAB=1',
      'ANKICORETECH_USE_GTEST=0',
      'ANKICORETECH_USE_OPENCV=1',
      'ANKICORETECH_EMBEDDED_USE_MATLAB=1',
      'ANKICORETECH_EMBEDDED_USE_GTEST=0',
      'ANKICORETECH_EMBEDDED_USE_OPENCV=1',
    ],
    
    'coretechDir' : ['../../coretech'],

    'common_mex_sources': [
      '<(coretechDir)/common/matlab/mex/mexWrappers.cpp',
      '<(coretechDir)/common/shared/src/matlabConverters.cpp',
      '<(coretechDir)/common/shared/src/sharedMatlabInterface.cpp',
      '<(coretechDir)/common/robot/src/matlabInterface.cpp'
    ],

    # TODO: Find this programmatically or pass it in
    'matlabRootDir' : ['/Applications/MATLAB_R2015a.app'],
    
    # TODO: Get this programmatically (via configure.py?) using a call to <(matlabRootDir)/bin/mexext
    'mexext' : ['mexmaci64'],  
   
    'compiler_flags': [
      '-Wno-unused-function',
      '-Wno-overloaded-virtual',
      '-Wno-deprecated-declarations',
      '-Wno-unused-variable',
    ],
    
    'compiler_c_flags' : [
      '-std=c11',
      '<@(compiler_flags)',
    ],
    
    'compiler_cpp_flags' : [
      '-std=c++11',
      '-stdlib=libc++',
      '<@(compiler_flags)'
    ],
    
    'linker_flags' : [
        '-g',
    ],

    # Set default ARCHS based on platform
    # We need an a new variables scope so that vars
    # can be overridden before they are used for conditionals.
    # see: http://src.chromium.org/svn/trunk/src/build/common.gypi
    'variables': {
      'arch_group%': 'universal',
    },

    
  }, # end variables

  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'ldflags': ['<@(linker_flags)'],
    'defines': ['<@(coretech_defines)'],
    'type': 'shared_library', # Mex files are really just shared libraries!
    'variables': {
      'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
    },
    'include_dirs': [
      '<(matlabRootDir)/extern/include',
      '../../include',
      '<(coretechDir)/common/include',
      '<(coretechDir)/vision/include',
      '<@(opencv_includes)',
    ],
    'libraries': [
      '<@(opencv_libs)',
      'libmx.dylib',
      'libmex.dylib',
      'libeng.dylib',
      'AppKit.framework',
      '<@(face_library_libs)',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'FRAMEWORK_SEARCH_PATHS':'$(SDKROOT)/System/Library/Frameworks',
      'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
      'CLANG_CXX_LIBRARY':'libc++',
      'DEBUG_INFORMATION_FORMAT': 'dwarf',
      'GCC_DEBUGGING_SYMBOLS': 'full',
      #'GCC_PREFIX_HEADER': '../../source/anki/basestation/basestation.pch',
      #'GCC_PRECOMPILE_PREFIX_HEADER': 'YES',
      'GCC_ENABLE_SYMBOL_SEPARATION': 'YES',
      # 'GENERATE_MASTER_OBJECT_FILE': 'YES',
      'EXECUTABLE_PREFIX': '',              # Note that mex files should not have "lib" prefix
      'EXECUTABLE_EXTENSION': '<(mexext)',  # Note that mex files use special extension, not .dylib
    },
    'configurations': {
      'Debug': {
          'cflags': ['-O0'],
          'cflags_cc': ['-O0'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-O0'],
            'OTHER_CPLUSPLUSFLAGS': ['-O0'],
            'LIBRARY_SEARCH_PATHS': [
              '<(matlabRootDir)/bin/maci64'
            ],
           },
          'defines': [
            '_LIBCPP_DEBUG=0',
            'DEBUG=1',
          ],
      },
      'Profile': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
            'LIBRARY_SEARCH_PATHS': [
              '<(matlabRootDir)/bin/maci64'
            ],
           },
          'defines': [
            'NDEBUG=1',
            'PROFILE=1',
          ],
      },
      'Release': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
            'LIBRARY_SEARCH_PATHS': [
              '<(matlabRootDir)/bin/maci64'
            ],
           },
          'defines': [
            'NDEBUG=1',
            'RELEASE=1',
          ],
      },
    },
  }, # end target_defaults



  'targets': [
    {
      'target_name': 'mexDetectFiducialMarkers',        
      'sources': [
        '<(coretechDir)/vision/robot/mex/mexDetectFiducialMarkers.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommon',
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-cti_gyp_path):ctiVision',
        '<(ce-cti_gyp_path):ctiVisionRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexDetectFiducialMarkers

    {
      'target_name': 'mexUnique',        
      'sources': [
        '<(coretechDir)/common/robot/mex/mexUnique.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommon',
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexUnique
    
    {
      'target_name': 'mexRegionprops',
      'sources': [
        '<(coretechDir)/vision/robot/mex/mexRegionprops.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommon',
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexRegionprops
    
    {
      'target_name': 'mexHist',
      'sources': [
        '<(coretechDir)/vision/robot/mex/mexHist.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommon',
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexHist
    
    {
      'target_name': 'mexRefineQuadrilateral',
      'sources': [
        '<(coretechDir)/vision/robot/mex/mexRefineQuadrilateral.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-cti_gyp_path):ctiVisionRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexRefineQuadrilateral
    
    {
      'target_name': 'mexClosestIndex',
      'sources': [
        '<(coretechDir)/vision/robot/mex/mexClosestIndex.cpp',
        '<@(common_mex_sources)',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-util_gyp_path):jsoncpp',
        '<(ce-util_gyp_path):util',
        '<(ce-util_gyp_path):utilEmbedded',
      ],
    }, # end mexClosestIndex
    
    {
      'target_name': 'mexCameraCapture',
      'sources': [
        '<(coretechDir)/vision/matlab/CameraCapture/mexCameraCapture.cpp',
        '<@(common_mex_sources)',
      ],
      'libraries': [
        '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
        '$(SDKROOT)/System/Library/Frameworks/CoreVideo.framework',
      ],
      'dependencies': [
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-cti_gyp_path):ctiVisionRobot',
      ],
    }, # end mexCameraCapture
    

  ] # end targets

} # Final closing brace

