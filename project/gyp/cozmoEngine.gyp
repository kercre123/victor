{
  'variables': {

    'engine_source': 'cozmoEngine.lst',
    'engine_test_source': 'cozmoEngine-test.lst',
    'energy_library_type': 'static_library',
    'ctrlLightCube_source': 'ctrlLightCube.lst',
    'ctrlRobot_source': 'ctrlRobot.lst',
    'ctrlViz_source': 'ctrlViz.lst',
    
    # TODO: should this be passed in, or shared?
    'coretech_defines': [
      'ANKICORETECH_USE_MATLAB=0',
      # 'ANKICORETECH_USE_GTEST=1',
      'ANKICORETECH_USE_OPENCV=1',
      'ANKICORETECH_EMBEDDED_USE_MATLAB=0',
      'ANKICORETECH_EMBEDDED_USE_GTEST=1',
      'ANKICORETECH_EMBEDDED_USE_OPENCV=1',
    ],

    # TODO: should this be passed in, or shared?
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
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_core.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_imgproc.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_highgui.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_calib3d.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_contrib.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_objdetect.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_video.dylib',
      '<(coretech_external_path)/build/opencv-2.4.8/lib/$(CONFIGURATION)$(EFFECTIVE_PLATFORM_NAME)/libopencv_features2d.dylib',
    ],

    'webots_includes': [
      '<(webots_path)/include/controller/cpp'
    ],

    'compiler_flags': [
      '-Wno-unused-function',
      '-Wno-overloaded-virtual',
      '-Wno-deprecated-declarations',
      '-Wno-unused-variable',
      # '-fdiagnostics-show-category=name',
      # '-Wall',
      # '-Woverloaded-virtual',
      # '-Werror',
      # '-Wundef',
      # '-Wheader-guard',
      # '-fsigned-char',
      # '-fvisibility-inlines-hidden',
      # '-fvisibility=default',
      # '-Wshorten-64-to-32',
      # '-Winit-self',
      # '-Wconditional-uninitialized',
      # '-Wno-deprecated-register',
      # '-Wformat',
      # '-Werror=format-security',
      # '-g',
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
        '-g'
    ],

    # Set default ARCHS based on platform
    # We need an a new variables scope so that vars
    # can be overridden before they are used for conditionals.
    # see: http://src.chromium.org/svn/trunk/src/build/common.gypi
    'variables': {
      'arch_group%': 'universal',
    },

    # Copy overridden vars into this scope
    'arch_group%': '<(arch_group)',
    'conditions': [
      ['OS=="ios" and arch_group=="universal"', {
        'target_archs%': ['armv7', 'arm64'],
      }],
      ['OS=="ios" and arch_group=="standard"', {
        'target_archs%': ['armv7']
      }],
      ['OS=="mac" and arch_group=="universal"', {
        'target_archs%': ['$(ARCHS_STANDARD_32_64_BIT)']
      }],
      ['OS=="mac" and arch_group=="standard"', {
        'target_archs%': ['$(ARCHS_STANDARD)']
      }],
      ['OS=="android"', {
        'target_archs%': ['armveabi-v7a'],
        'compiler_flags': [
          '--sysroot=<(ndk_root)/platforms/android-18/arch-arm',
          '-DANDROID=1',
          '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
          '-fpic',
          '-ffunction-sections',
          '-funwind-tables',
          '-fstack-protector',
          '-no-canonical-prefixes',
          '-fno-integrated-as',
          '-target', 'armv7-none-linux-androideabi',
          '-march=armv7-a',
          '-mfloat-abi=softfp',
          '-mfpu=vfpv3-d16',
          '-mthumb',
          '-fomit-frame-pointer',
          '-fno-strict-aliasing',
          '-I<(ndk_root)/sources/cxx-stl/llvm-libc++/libcxx/include',
          '-I<(ndk_root)/sources/cxx-stl/llvm-libc++/../llvm-libc++abi/libcxxabi/include',
          '-I<(ndk_root)/sources/cxx-stl/llvm-libc++/../../android/support/include',
          '-I<(ndk_root)/platforms/android-18/arch-arm/usr/include',
        ],
        'linker_flags': [
            '--sysroot=<(ndk_root)/platforms/android-18/arch-arm',
            '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
            '-no-canonical-prefixes',
            '-target armv7-none-linux-androideabi',
            '-Wl,--fix-cortex-a8',
            '-Wl,--no-undefined',
            '-Wl,-z,noexecstack',
            '-Wl,-z,relro',
            '-Wl,-z,now',
            '-mthumb',
            '-L<(ndk_root)/platforms/android-18/arch-arm/usr/lib',
            '-L<(ndk_root)/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a',
            '-lgcc',
            '-lc',
            '-lm',
            '-lc++_shared',
            '-lGLESv2',
            '-llog',
        ],
      },
      { # not android
        'SHARED_LIB_DIR': '', #bogus, just to make the mac / ios builds happy
      },
      ],
      ['OS=="ios"', {
        'compiler_flags': [
        '-fobjc-arc',
        ]
      }],
      ['OS=="ios" or OS=="mac"', {
        'linker_flags': [
          '-std=c++11',
          '-stdlib=libc++',
          '-lpthread',
        ]
      }],
    ],
  },

  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'ldflags': ['<@(linker_flags)'],
    'defines': ['<@(coretech_defines)'],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      # 'FRAMEWORK_SEARCH_PATHS':'../../libs/framework/',
      'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
      'CLANG_CXX_LIBRARY':'libc++',
      'DEBUG_INFORMATION_FORMAT': 'dwarf',
      'GCC_DEBUGGING_SYMBOLS': 'full',
      #'GCC_PREFIX_HEADER': '../../source/anki/basestation/basestation.pch',
      #'GCC_PRECOMPILE_PREFIX_HEADER': 'YES',
      'GCC_ENABLE_SYMBOL_SEPARATION': 'YES',
      # 'GENERATE_MASTER_OBJECT_FILE': 'YES',
    },
    'configurations': {
      'Debug': {
          'cflags': ['-O0'],
          'cflags_cc': ['-O0'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-O0'],
            'OTHER_CPLUSPLUSFLAGS': ['-O0'],
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
           },
          'defines': [
            'NDEBUG=1',
            'RELEASE=1',
          ],
      },
    },
    'conditions': [    
      [
        "OS=='ios'",
        {
          'defines': [
            'ANKI_IOS_BUILD=1',
          ],
        },
      ],
    ],
  },

  'conditions': [    
    [
      "OS=='android'",
      {
        'defines': [
          'ANDROID=1',
        ],
      },
    ],
    [
      "OS=='ios'",
      {
        'xcode_settings': {
          'SDKROOT': 'iphoneos',
          'VALID_ARCHS' : ['<@(target_archs)']
          #'TARGETED_DEVICE_FAMILY': '1,2',
          #'CODE_SIGN_IDENTITY': 'iPhone Developer',
          #'IPHONEOS_DEPLOYMENT_TARGET': '5.1',
        },
      },
    ],



    # UNITTEST CRAP HERE
    # UNITTEST CRAP HERE
    # UNITTEST CRAP HERE
    # UNITTEST CRAP HERE

    [
      "OS=='mac'",
      {
        'target_defaults': {
          'variables': {
            'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
          },
          'xcode_settings': {
            'ARCHS': [ '>@(mac_target_archs)' ],
            'SDKROOT': 'macosx',
            # 'MACOSX_DEPLOYMENT_TARGET': '10.9', # latest
          },
        },


        'targets': [
          {
            'target_name': 'webotsCtrlLightCube',
            'type': 'executable',
            'include_dirs': [
              '../../robot/include',
              '../../include',
              '../../simulator/include',
              '<@(webots_includes)',
            ],
            'dependencies': [
              '<(ce-cti_gyp_path):ctiCommonRobot',
            ],
            'sources': [ '<!@(cat <(ctrlLightCube_source))' ],
            'defines': [
              'COZMO_ROBOT',
              'SIMULATOR'
            ],
            'libraries': [
              '<(webots_path)/lib/libCppController.dylib',
            ],
          }, # end controller Block

          {
            'target_name': 'webotsCtrlViz',
            'type': 'executable',
            'include_dirs': [
              # '../../robot/include',
              '../../include',
              # '../../simulator/include',
              '<@(webots_includes)',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              '<(ce-cti_gyp_path):ctiCommon',
              '<(ce-cti_gyp_path):ctiVision',
              '<(ce-cti_gyp_path):ctiMessaging',
              '<(ce-util_gyp_path):util',
            ],
            'sources': [ '<!@(cat <(ctrlViz_source))' ],
            'defines': [
              # 'COZMO_ROBOT',
              # 'SIMULATOR'
            ],
            'libraries': [
              '<(webots_path)/lib/libCppController.dylib',
              '<@(opencv_libs)',
            ],
          }, # end controller viz

          {
            'target_name': 'webotsCtrlRobot',
            'type': 'executable',
            'include_dirs': [
              '../../robot/include',
              '../../include',
              '../../simulator/include',
              '<@(webots_includes)',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              '<(ce-cti_gyp_path):ctiCommonRobot',
              '<(ce-cti_gyp_path):ctiVisionRobot',
              '<(ce-cti_gyp_path):ctiMessagingRobot',
              '<(ce-cti_gyp_path):ctiPlanningRobot',
              '<(ce-util_gyp_path):utilEmbedded',
            ],
            'sources': [ '<!@(cat <(ctrlRobot_source))' ],
            'defines': [
              'COZMO_ROBOT',
              'SIMULATOR'
            ],
            'libraries': [
              '<(webots_path)/lib/libCppController.dylib',
              '<@(opencv_libs)',
            ],
          }, # end controller Robot

          {
            'target_name': 'cozmoEngineUnitTest',
            'type': 'executable',
            'include_dirs': [
              '../../basestation/test',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              'cozmoEngine',
              '<(ce-cti_gyp_path):ctiCommon',
              '<(ce-cti_gyp_path):ctiCommonRobot',
              '<(ce-cti_gyp_path):ctiMessaging',
              '<(ce-cti_gyp_path):ctiPlanning',
              '<(ce-cti_gyp_path):ctiVision',
              '<(ce-cti_gyp_path):ctiVisionRobot',
              '<(ce-util_gyp_path):jsoncpp',
              '<(ce-util_gyp_path):util',
            ],
            'sources': [ '<!@(cat <(engine_test_source))' ],
            'sources/': [
              ['exclude', 'run_pc_embeddedTests.cpp'],
              ['exclude', 'run_m4_embeddedTests.cpp'],
              ['exclude', 'resaveBlockImages.m'],
            ],
            'libraries': [
              '<(ce-gtest_path)/gtest.framework',
              '<@(opencv_libs)',
            ],
            'copies': [
              {
                'files': [
                  '<(ce-gtest_path)/gtest.framework',
                ],
                'destination': '<(PRODUCT_DIR)',
              },
            ],
          }, # end unittest target



        ], # end targets
      },
    ] # end if mac


  ], #end conditions






  # CORE TARGETS HERE
  # CORE TARGETS HERE
  # CORE TARGETS HERE
  # CORE TARGETS HERE




  'targets': [

    {
      'target_name': 'cozmoEngine',
      'sources': [ "<!@(cat <(engine_source))" ],
      'sources/': [
        ['exclude', 'bleRobotManager.mm'],
        ['exclude', 'bleComms.mm'],
      ],
      'include_dirs': [
        '../../basestation/src',
        '../../basestation/include',
        '../../include',
        '<@(opencv_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../basestation/include',
          '../../include',
        ],
        'defines': [
          'COZMO_BASESTATION'
        ],
      },
      'defines': [
        'COZMO_BASESTATION'
      ],
      'dependencies': [
        '<(ce-util_gyp_path):util',
        '<(ce-cti_gyp_path):ctiCommon',
        '<(ce-cti_gyp_path):ctiMessaging',
        '<(ce-cti_gyp_path):ctiPlanning',
        '<(ce-cti_gyp_path):ctiVision',
      ],
      'type': '<(energy_library_type)',
    },
    


  ] # end targets

}
