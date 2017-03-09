{
  'includes': [
    'face-library.gypi',
    'opencv.gypi',
    '../../../project/gyp/build-variables.gypi'
  ],
  
  'variables': {

    'common_library_type': 'static_library',
    'common_robot_library_type': 'static_library',
    'vision_library_type': 'static_library',
    'vision_robot_library_type': 'static_library',
    'planning_library_type': 'static_library',
    'planning_robot_library_type': 'static_library',
    'messaging_library_type': 'static_library',
    'messaging_robot_library_type': 'static_library',

    'common_source': 'common.lst',
    'common_clad': 'common-clad.lst',
    'common_test_source': 'common-test.lst',
    'common_shared_test_source': 'common-shared-test.lst',
    'common_robot_source': 'common-robot.lst',
    
    'vision_source': 'vision.lst',
    'vision_clad': 'vision-clad.lst',
    'vision_test_source': 'vision-test.lst',
    'vision_robot_source': 'vision-robot.lst',
    
    'planning_source': 'planning.lst',
    'planning_standalone_source': 'planning-standalone.lst',
    'planning_test_source': 'planning-test.lst',
    'planning_robot_source': 'planning-robot.lst',
    
    'messaging_source': 'messaging.lst',
    
    'vision_generated_source': '../../generated/clad/vision/vision.lst',
    'common_generated_source': '../../generated/clad/common/common.lst',
    
    'messaging_robot_source': 'messaging-robot.lst',

    # TODO: should this be passed in, or shared?
    'coretech_defines': [
      'ANKICORETECH_USE_MATLAB=0',
      # 'ANKICORETECH_USE_GTEST=1',
      'ANKICORETECH_USE_OPENCV=1',
      'ANKICORETECH_EMBEDDED_USE_MATLAB=0',
      'ANKICORETECH_EMBEDDED_USE_GTEST=1',
      'ANKICORETECH_EMBEDDED_USE_OPENCV=1',
    ],

    'compiler_flags': [
      '-DJSONCPP_USING_SECURE_MEMORY=0',
      '-Wno-deprecated-declarations', # Supressed until system() usage is removed
      '-fdiagnostics-show-category=name',
      '-Wall',
      '-Woverloaded-virtual',
      '-Werror',
      '-Wundef',
      '-Wheader-guard',
      '-fsigned-char',
      '-fvisibility-inlines-hidden',
      '-fvisibility=default',
      '-Wshorten-64-to-32',
      '-Winit-self',
      '-Wconditional-uninitialized',
      # '-Wno-deprecated-register', # Disabled until this warning actually needs to be supressed
      '-Wformat',
      '-Werror=format-security',
      '-g',
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
      ['OS=="linux"', {
        'target_archs%': ['x64'],
      }],
      ['OS=="android"', {
        'target_archs%': ['armveabi-v7a'],
       	'target_cpu': ['arm'],
        'compiler_flags': [
          '--sysroot=<(ndk_root)/platforms/android-18/arch-arm',
          '-DANDROID=1',
          '-D__ARM_NEON=1',
          '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64',
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
          '-I<(ndk_root)/sources/cxx-stl/llvm-libc++/include',
          '-I<(ndk_root)/sources/cxx-stl/llvm-libc++abi/include',
          '-I<(ndk_root)/sources/android/support/include',
          '-I<(ndk_root)/platforms/android-18/arch-arm/usr/include',
        ],
        'linker_flags': [
            '--sysroot=<(ndk_root)/platforms/android-18/arch-arm',
            '-gcc-toolchain', '<(ndk_root)/toolchains/arm-linux-androideabi-4.9/prebuilt/darwin-x86_64',
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
            '-L<(coretech_external_path)/okaoVision/lib/Android/armeabi-v7a',
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
      ['OS=="ios" or OS=="mac" or OS=="linux"', {
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
    'include_dirs': [
      '../../tools/message-buffers/support/cpp/include',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'conditions': [
        ['face_library=="faciometric"', {
           'FRAMEWORK_SEARCH_PATHS': '<(face_library_path)/IntraFace_126_iOS_Anki/Library',
        }],
      ],
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
            'LIBRARY_SEARCH_PATHS': [
                '<@(opencv_lib_search_path_debug)',
                '<(face_library_lib_path)'
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
              '<@(opencv_lib_search_path_release)',
              '<(face_library_lib_path)'
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
              '<@(opencv_lib_search_path_release)',
              '<(face_library_lib_path)'
            ],
           },
          'defines': [
            'NDEBUG=1',
            'RELEASE=1',
          ],
      },
      'Shipping': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
            'LIBRARY_SEARCH_PATHS': [
              '<@(opencv_lib_search_path_release)',
              '<(face_library_lib_path)'
            ],
           },
          'defines': [
            'NDEBUG=1',
            'SHIPPING=1',
          ],
      },
    },
    'conditions': [    
      [
        "OS=='ios'",
        {
          'defines': [
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
            'mac_target_archs%': [ '<@(target_archs)' ]
          },
          'xcode_settings': {
            'ARCHS': [ '>@(mac_target_archs)' ],
            'SDKROOT': 'macosx',
            'MACOSX_DEPLOYMENT_TARGET': '<(macosx_deployment_target)',
            'LD_RUNPATH_SEARCH_PATHS': '@loader_path',
            'LD_DYLIB_INSTALL_NAME': '@loader_path/$(EXECUTABLE_PATH)',
            'FRAMEWORK_SEARCH_PATHS':'<(cti-gtest_path)',
            }
        },


        'targets': [
          {
            'target_name': 'ctiUnitTest',
            'type': 'executable',
            'variables': {
              'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../common/shared/src',
              '../../common/basestation/test',
              '../../planning/basestation/test',
              '../../vision/basestation/test',
              '../../../robot/include',
              '<@(opencv_includes)',
            ],
            'defines': [
              'TEST_DATA_PATH=<(cti-cozmo_engine_path)/coretech/'
            ],
            'dependencies': [
              'ctiCommon',
              'ctiVision',
              'ctiPlanning',
              '<(cti-util_gyp_path):util',
              '<(cti-util_gyp_path):jsoncpp',
            ],
            'sources': [ 
              '<!@(cat <(common_test_source))',
              '<!@(cat <(common_shared_test_source))',
              '<!@(cat <(vision_test_source))',
              '<!@(cat <(planning_test_source))',
            ],
            'sources/': [
              ['exclude', 'run_coreTechCommonSharedTests.cpp'],
              ['exclude', 'run_coreTechVisionTests.cpp'],
              ['exclude', 'run_coreTechPlanningTests.cpp'],
              ['exclude', 'run_coreTechPlanningStandalone.cpp'],
            ],
            'xcode_settings': {
              'FRAMEWORK_SEARCH_PATHS':'<(cti-gtest_path)',
            },
            'libraries': [
              '<(cti-gtest_path)/gtest.framework',
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',        # Why is this needed for ctiUnitTest?
              '<@(opencv_libs)',
            ],
            'actions' : [
              {
                'action_name': 'create_symlink_ctiUnitTestFaceLibraryLibs',
                'inputs': [ ],
                'outputs': [ ],
                'conditions': [
                  ['face_library=="faciometric"', {
                    'action': [
                      'ln',
                      '-s',
                      '-h',
                      '-f',
                      '<(face_library_lib_path)',
                      '<(PRODUCT_DIR)/',
                    ],
                  }],
                  ['face_library=="facesdk"', {
                    'action': [
                      'ln',
                      '-s',
                      '-f',
                      '<(face_library_lib_path)/libfsdk.dylib',
                      '<(PRODUCT_DIR)',
                    ],
                  }],
                  ['face_library=="opencv" or face_library=="okao"', {
                    'action': [
                    'echo',
                    'dummyOpenCVCTIAction',
                    ],
                  }],
                ], # conditions
              },
            ] # actions
          }, # end unittest target

          {
            'target_name': 'ctiPlanningStandalone',
            'type': 'executable',
            'variables': {
              'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../common/shared/src',
              '../../common/basestation/test',
              '../../planning/basestation/test',
              '../../vision/basestation/test',
              '../../../robot/include',
              '<@(opencv_includes)',
            ],
            'defines': [
              'TEST_DATA_PATH=<(cti-cozmo_engine_path)/coretech/'
            ],
            'dependencies': [
              'ctiCommon',
              'ctiVision',
              'ctiPlanning',
              '<(cti-util_gyp_path):util',
              '<(cti-util_gyp_path):jsoncpp',
            ],
            'sources': [ 
              '<!@(cat <(common_test_source))',
              '<!@(cat <(common_shared_test_source))',
              '<!@(cat <(vision_test_source))',
              '<!@(cat <(planning_test_source))',
              '<!@(cat <(planning_standalone_source))',
            ],
            'sources/': [
              ['exclude', 'run_coreTechCommonSharedTests.cpp'],
              ['exclude', 'run_coreTechVisionTests.cpp'],
              ['exclude', 'run_coreTechPlanningTests.cpp'],
              ['exclude', 'run_coreTechCommonTests.cpp'],
              ['exclude', '.*/test/.*'],
              ['include', 'run_coreTechPlanningStandalone.cpp'],
            ],
            'xcode_settings': {
            },
            'libraries': [
              '<@(opencv_libs)',
            ],
            'actions' : [
              {
                'action_name': 'create_symlink_ctiUnitTestFaceLibraryLibs',
                'inputs': [ ],
                'outputs': [ ],
                'conditions': [
                  ['face_library=="faciometric"', {
                    'action': [
                      'ln',
                      '-s',
                      '-h',
                      '-f',
                      '<(face_library_lib_path)',
                      '<(PRODUCT_DIR)/',
                    ],
                  }],
                  ['face_library=="facesdk"', {
                    'action': [
                      'ln',
                      '-s',
                      '-f',
                      '<(face_library_lib_path)/libfsdk.dylib',
                      '<(PRODUCT_DIR)',
                    ],
                  }],
                  ['face_library=="opencv" or face_library=="okao"', {
                    'action': [
                      'echo',
                      'dummyOpenCVCTIAction',
                    ],
                  }],
                ], # conditions
              },
            ] # actions
          },

        ], # end targets
      },
    ], # end if mac


    [
      "OS=='linux'",
      {
        'target_defaults': {
          'variables': {
            'linux_target_archs%': [ '<@(target_archs)' ]
          },
        },


        'targets': [
          {
            'target_name': 'ctiUnitTest',
            'type': 'executable',
            'variables': {
              'linux_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../common/shared/src',
              '../../common/basestation/test',
              '../../planning/basestation/test',
              '../../vision/basestation/test',
              '../../../robot/include',
              '<@(opencv_includes)',
            ],
            'defines': [
              'TEST_DATA_PATH=<(cti-cozmo_engine_path)/coretech/'
            ],
            'dependencies': [
              'ctiCommon',
              'ctiVision',
              'ctiPlanning',
              '<(cti-util_gyp_path):util',
              '<(cti-util_gyp_path):jsoncpp',
            ],
            'sources': [
              '<!@(cat <(common_test_source))',
              '<!@(cat <(common_shared_test_source))',
              '<!@(cat <(vision_test_source))',
              '<!@(cat <(planning_test_source))',
            ],
            'sources/': [
              ['exclude', 'run_coreTechCommonSharedTests.cpp'],
              ['exclude', 'run_coreTechVisionTests.cpp'],
              ['exclude', 'run_coreTechPlanningTests.cpp'],
              ['exclude', 'run_coreTechPlanningStandalone.cpp'],
            ],
            'xcode_settings': {
              'FRAMEWORK_SEARCH_PATHS':'<(cti-gtest_path)',
            },
            'libraries': [
              '<(cti-gtest_path)/gtest.framework',
              '<@(opencv_libs)',
            ],
            'actions' : [
              {
                'action_name': 'create_symlink_ctiUnitTestFaceLibraryLibs',
                'inputs': [ ],
                'outputs': [ ],
                'conditions': [
                  ['face_library=="faciometric"', {
                    'action': [
                      'ln',
                      '-s',
                      '-h',
                      '-f',
                      '<(face_library_lib_path)',
                      '<(PRODUCT_DIR)/',
                    ],
                  }],
                  ['face_library=="facesdk"', {
                    'action': [
                      'ln',
                      '-s',
                      '-f',
                      '<(face_library_lib_path)/libfsdk.dylib',
                      '<(PRODUCT_DIR)',
                    ],
                  }],
                  ['face_library=="opencv"', {
                    'action': [
                    'echo',
                    'dummyOpenCVCTIAction',
                    ],
                  }],
                ], # conditions
              },
            ] # actions
          }, # end unittest target

          {
            'target_name': 'ctiPlanningStandalone',
            'type': 'executable',
            'variables': {
              'linux_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../common/shared/src',
              '../../common/basestation/test',
              '../../planning/basestation/test',
              '../../vision/basestation/test',
              '../../../robot/include',
              '<@(opencv_includes)',
            ],
            'defines': [
              'TEST_DATA_PATH=<(cti-cozmo_engine_path)/coretech/'
            ],
            'dependencies': [
              'ctiCommon',
              'ctiVision',
              'ctiPlanning',
              '<(cti-util_gyp_path):util',
              '<(cti-util_gyp_path):jsoncpp',
            ],
            'sources': [
              '<!@(cat <(common_test_source))',
              '<!@(cat <(common_shared_test_source))',
              '<!@(cat <(vision_test_source))',
              '<!@(cat <(planning_test_source))',
              '<!@(cat <(planning_standalone_source))',
            ],
            'sources/': [
              ['exclude', 'run_coreTechCommonSharedTests.cpp'],
              ['exclude', 'run_coreTechVisionTests.cpp'],
              ['exclude', 'run_coreTechPlanningTests.cpp'],
              ['exclude', 'run_coreTechCommonTests.cpp'],
              ['exclude', '.*/test/.*'],
              ['include', 'run_coreTechPlanningStandalone.cpp'],
            ],
            'xcode_settings': {
            },
            'libraries': [
              '<@(opencv_libs)',
            ],
            'actions' : [
              {
                'action_name': 'create_symlink_ctiUnitTestFaceLibraryLibs',
                'inputs': [ ],
                'outputs': [ ],
                'conditions': [
                  ['face_library=="faciometric"', {
                    'action': [
                      'ln',
                      '-s',
                      '-h',
                      '-f',
                      '<(face_library_lib_path)',
                      '<(PRODUCT_DIR)/',
                    ],
                  }],
                  ['face_library=="facesdk"', {
                    'action': [
                      'ln',
                      '-s',
                      '-f',
                      '<(face_library_lib_path)/libfsdk.dylib',
                      '<(PRODUCT_DIR)',
                    ],
                  }],
                  ['face_library=="opencv"', {
                    'action': [
                      'echo',
                      'dummyOpenCVCTIAction',
                    ],
                  }],
                ], # conditions
              },
            ] # actions
          },

        ], # end targets
      },
    ] # end if linux


  ], #end conditions





  # CORE TARGETS HERE
  # CORE TARGETS HERE
  # CORE TARGETS HERE
  # CORE TARGETS HERE


  'targets': [

    {
      'target_name': 'ctiCommon',
      'sources': [
        '<!@(cat <(common_source))',
        '<!@(cat <(common_clad))',
      ],
      'include_dirs': [
        '../../common/basestation/src',
        '../../common/include',
        '../../generated/clad/common',
        '<@(opencv_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../common/include',
          '../../generated/clad/common',
        ],
      },
      'defines': [
        'SYSTEM_ROOT_PATH=<(cti-cozmo_engine_path)'
      ],
      'dependencies': [
        '<(cti-util_gyp_path):jsoncpp',
        '<(cti-util_gyp_path):util',
      ],
      'type': '<(common_library_type)',
      'conditions': [
        # Add common generated source to prevent linker errors for mex (and other?) builds,
        # but this causes "duplicate symbol" errors when linking for Android. So only do it for Mac.
        ['OS=="mac"',     {'sources': ['<!@(cat <(common_generated_source))'],}],
        ['OS!="mac"',     {'sources/': [['exclude', '_osx\\.']]}],
        ['OS!="ios"',     {'sources/': [['exclude', '_ios\\.|_iOS\\.']]}],
        ['OS!="android"', {'sources/': [['exclude', '_android\\.']]}],
        ['OS!="linux"',   {'sources/': [['exclude', '_linux\\.']]}],
      ],

    },

    {
      'target_name': 'ctiCommonRobot',
      'sources': [ '<!@(cat <(common_robot_source))' ],
      'conditions': [
        ['OS=="android"',{
          'sources/': [
            ['exclude', 'radians|utilities_shared|externalInterface']
          ]
        }],
      ], #'conditions'
      'include_dirs': [
        '../../common/robot/src',
        '../../common/include',
        '<@(opencv_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../common/include',
        ],
      },
      'dependencies': [
        '<(cti-util_gyp_path):util'
      ],
      'type': '<(common_robot_library_type)',
    },

    {
      'target_name': 'ctiMessaging',
      'sources': [ 
        '<!@(cat <(messaging_source))',
        # Why can we include these here and be fine with Android link step, but not for ctiVision/ctiCommon??
        # Maybe it's ok as long as we only do this on _one_ of the three?
        '<!@(cat <(vision_generated_source))',
        '<!@(cat <(common_generated_source))',
      ],
      'conditions': [
        ['OS=="android"',{
          'sources/': [
            ['exclude', 'utilMessaging|externalInterface|vision|common'], #TODO: COZMO-1271
          ]
        }],
      ], #'conditions'
      'include_dirs': [
        '../../messaging/basestation/src',
        '../../messaging/include',
        '../../generated/clad/vision',
        '../../generated/clad/common',
        '../../../generated/clad/engine', # TODO: Fix. Reaches out to engine :-(
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../messaging/include',
          '../../../generated/clad/engine',
        ],
      },
      'dependencies': [
        'ctiCommon',
        '<(cti-util_gyp_path):util',
      ],
      'type': '<(messaging_library_type)',
    },

    {
      'target_name': 'ctiMessagingRobot',
      'sources': [ '<!@(cat <(messaging_robot_source))' ],
      'include_dirs': [
        '../../messaging/robot/src',
        '../../messaging/include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../messaging/include',
        ],
      },
      'type': '<(messaging_robot_library_type)',
    },
    
    {
      'target_name': 'ctiPlanning',
      'sources': [ '<!@(cat <(planning_source))' ],
      'include_dirs': [
        '../../planning/basestation/src',
        '../../planning/include',
        '../../../robot/include',
        '<@(opencv_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../planning/include',
        ],
        'defines': [
          'CORETECH_BASESTATION'
        ],
      },
      'dependencies': [
        'ctiCommon',
        '<(cti-util_gyp_path):jsoncpp',
        '<(cti-util_gyp_path):util',
      ],
      'defines': [
        'CORETECH_BASESTATION'
      ],
      'type': '<(planning_library_type)',
    },

    {
      'target_name': 'ctiPlanningRobot',
      'sources': [ '<!@(cat <(planning_robot_source))' ],
      'include_dirs': [
        '../../planning/robot/src',
        '../../planning/include',
        '../../../robot/include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../planning/include',
        ],
        'defines': [
          'CORETECH_ROBOT'
        ],
      },
      'dependencies': [
        'ctiCommonRobot',
        '<(cti-util_gyp_path):util'
      ],
      'defines': [
        'CORETECH_ROBOT'
      ],
      'type': '<(planning_robot_library_type)',
    },
    {
      'target_name': 'ctiVision',
      'sources': [
        '<!@(cat <(vision_source))',
        '<!@(cat <(vision_clad))',
      ],
      'conditions': [
        ['OS=="mac"', {
          # Add common/vision generated source to prevent linker errors for mex (and other?) builds,
          # but this causes "duplicate symbol" errors when linking for Android. So only do it for Mac.
          'sources': [
            '<!@(cat <(common_generated_source))',
            '<!@(cat <(vision_generated_source))',
          ],
        }],
      ],
      'include_dirs': [
        '../../vision/basestation/src',
        '../../vision/include',
        '../../generated/clad/vision',
        '<@(opencv_includes)',
        '<(coretech_external_path)/matconvnet/matlab/src/bits',
        '<@(face_library_includes)',
      ],
      'libraries': [
        '<@(face_library_libs)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../vision/include',
          '../../generated/clad/vision',
        ],
      },
      'dependencies': [
        'ctiCommon',
        '<(cti-util_gyp_path):jsoncpp',
        '<(cti-util_gyp_path):util',
      ],
      'type': '<(vision_library_type)',
    },

    {
      'target_name': 'ctiVisionRobot',
      'sources': [ '<!@(cat <(vision_robot_source))' ],
      'include_dirs': [
        '../../vision/robot/src',
        '../../vision/include',
        # TODO: REMOVE THIS
        '<@(opencv_includes)',
        '../../../include'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../vision/include',
        ],
      },
      'dependencies': [
        'ctiCommonRobot',
        '<(cti-util_gyp_path):jsoncpp',
        '<(cti-util_gyp_path):util'
      ],
      'type': '<(vision_robot_library_type)',
    },


  ] # end targets

}
