{
  'includes': [
    '../../coretech/project/gyp/face-library.gypi',
    '../../coretech/project/gyp/opencv.gypi',
  ],
  
  'variables': {

    'engine_source': 'cozmoEngine.lst',
    'engine_test_source': 'cozmoEngine-test.lst',
    'engine_library_type': 'static_library',
    'ctrlLightCube_source': 'ctrlLightCube.lst',
    'ctrlRobot_source': 'ctrlRobot.lst',
    'ctrlViz_source': 'ctrlViz.lst',
    'clad_source': 'clad.lst',
    'pluginPhysics_source': 'pluginPhysics.lst',
    
    # TODO: should this be passed in, or shared?
    'coretech_defines': [
      'ANKICORETECH_USE_MATLAB=0',
      # 'ANKICORETECH_USE_GTEST=1',
      'ANKICORETECH_USE_OPENCV=1',
      'ANKICORETECH_EMBEDDED_USE_MATLAB=0',
      'ANKICORETECH_EMBEDDED_USE_GTEST=1',
      'ANKICORETECH_EMBEDDED_USE_OPENCV=1',
    ],
    
    'pocketsphinx_includes':[
      '<(coretech_external_path)/pocketsphinx/sphinxbase/include',
      '<(coretech_external_path)/pocketsphinx/pocketsphinx/include',
    ],

    'cte_lib_search_path_mac_debug': [
      '<(coretech_external_path)/pocketsphinx/pocketsphinx/generated/mac/DerivedData/Debug',
    ],

    'cte_lib_search_path_mac_release': [
      '<(coretech_external_path)/pocketsphinx/pocketsphinx/generated/mac/DerivedData/Release',
    ],

    'cte_lib_search_path_ios_debug': [
      '<(coretech_external_path)/pocketsphinx/pocketsphinx/generated/ios/DerivedData/Debug-iphoneos',
    ],

    'cte_lib_search_path_ios_release': [
      '<(coretech_external_path)/pocketsphinx/pocketsphinx/generated/ios/DerivedData/Release-iphoneos',
    ],

    'webots_includes': [
      '<(webots_path)/include/controller/cpp',
      '<(webots_path)/include/ode',
      '<(webots_path)/include',
    ],
  
    'compiler_flags': [
      '-Wno-deprecated-declarations', # Supressed until system() usage is removed
      '-fdiagnostics-show-category=name',
      '-Wall',
      '-Woverloaded-virtual',
      '-Werror',
      # '-Wundef', # Disabled until define usage is refactored to code standards (ANS: common.h ODE inside Webots fails this)
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
      '<@(compiler_flags)',
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
        ],
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
      'ENABLE_BITCODE': 'NO',
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
          'conditions': [
            ['OS=="ios"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_ios_debug)', '<(webots_path)/lib/' ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_mac_debug)', '<(webots_path)/lib/' ],
              },
            }],
          ],
          'cflags': ['-O0'],
          'cflags_cc': ['-O0'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-O0'],
            'OTHER_CPLUSPLUSFLAGS': ['-O0'],
            'OTHER_LDFLAGS': ['<@(linker_flags)'],
          },
          'defines': [
            '_LIBCPP_DEBUG=0',
            'DEBUG=1',
          ],
      },
      'Profile': {
          'conditions': [
            ['OS=="ios"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_ios_release)', '<(webots_path)/lib/' ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_mac_release)', '<(webots_path)/lib/' ],
              },
            }],
          ],
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
            'OTHER_LDFLAGS': ['<@(linker_flags)'],
          },
          'defines': [
            'NDEBUG=1',
            'PROFILE=1',
          ],
      },
      'Release': {
          'conditions': [
            ['OS=="ios"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_ios_release)', '<(webots_path)/lib/' ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [ '<@(cte_lib_search_path_mac_release)', '<(webots_path)/lib/' ],
              },
            }],
          ],
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
            'OTHER_LDFLAGS': ['<@(linker_flags)'],
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
            'target_name': 'cozmo_physics',
            'type': 'shared_library',
            'include_dirs': [
              '../../include',
              '<@(webots_includes)',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              '<(ce-cti_gyp_path):ctiCommon',
              '<(ce-cti_gyp_path):ctiMessaging',
              '<(ce-util_gyp_path):util',
            ],
            'sources': [ 
              '<!@(cat <(pluginPhysics_source))',
            ],
            'defines': [
              'MACOS',
            ],
            'libraries': [
              'libCppController.dylib',
              '<@(opencv_libs)',
              '$(SDKROOT)/System/Library/Frameworks/OpenGL.framework',
              '$(SDKROOT)/System/Library/Frameworks/GLUT.framework',              
            ],
          }, # end cozmo_physics
 
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
              'libCppController.dylib',
            ],
          }, # end controller Block

          {
            'target_name': 'webotsCtrlViz',
            'type': 'executable',
            'include_dirs': [
              '../../include',
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
              'libCppController.dylib',
              '<@(opencv_libs)',
            ],
            'conditions': [
              # For some reason, need to link directly against FacioMetric libs
              # when using them for recognition, which also means they have to be
              # present (symlinked) in the executable dir
              ['face_library == "faciometric"', {
                'libraries': [
                  '<@(face_library_libs)',
                ],
                'actions' : [
                  {
                    'action_name': 'create_symlink_webotsCtrlViz_faciometricLibs',
                      'inputs': [ ],
                      'outputs': [ ],
                      'action': [
                        'ln',
                        '-s',
                        '-h',
                        '-f',
                        '<(face_library_lib_path)',
                        '../../simulator/controllers/webotsCtrlViz/',
                      ],
                  },
                ], # actions
              }], # conditions
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
              'libCppController.dylib',
              '<@(opencv_libs)',
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
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
            'xcode_settings': {
              'FRAMEWORK_SEARCH_PATHS':'<(ce-gtest_path)',
            },
            'libraries': [
              '<(ce-gtest_path)/gtest.framework',
              '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
              '<@(opencv_libs)',
              '<@(face_library_libs)',
            ],
            'actions': [
              # { # in engine only mode, we do not know where the assets are
              #   'action_name': 'create_symlink_resources_assets',
              #   'inputs': [
              #     '<(cozmo_asset_path)',
              #   ],
              #   'outputs': [
              #     '<(PRODUCT_DIR)/resources/assets',
              #   ],
              #   'action': [
              #     'ln',
              #     '-s',
              #     '-f',
              #     '-h',
              #     '<@(_inputs)',
              #     '<@(_outputs)',
              #   ],
              # },
              {
                'action_name': 'create_symlink_resources_configs',
                'inputs': [
                  '<(cozmo_engine_path)/resources/config',
                ],
                'outputs': [
                  '<(PRODUCT_DIR)/resources/config',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '-h',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_resources_test',
                'inputs': [
                  '<(cozmo_engine_path)/resources/test',
                ],
                'outputs': [
                  '<(PRODUCT_DIR)/resources/test',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '-h',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_resources_pocketsphinx',
                'inputs': [
                  '<(coretech_external_path)/pocketsphinx/pocketsphinx/model/en-us',
                ],
                'outputs': [
                  '<(PRODUCT_DIR)/resources/pocketsphinx',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '-h',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_engineUnitTestfaceLibraryLibs',
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
                ], # conditions
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
      'sources': [ 
        '<!@(cat <(engine_source))',
        '<!@(cat <(clad_source))',
      ],
      'sources/': [
        ['exclude', 'bleRobotManager.mm'],
        ['exclude', 'bleComms.mm'],
      ],
      'include_dirs': [
        '../../basestation/src',
        '../../basestation/include',
        '../../include',
        '../../generated/clad/engine',
        '<@(opencv_includes)',
        '<@(pocketsphinx_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../basestation/include',
          '../../include',
          '../../generated/clad/engine',
          '../../basestation/src',
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
        '<(ce-cti_gyp_path):ctiCommonRobot',
        '<(ce-cti_gyp_path):ctiVisionRobot',
      ],
      'type': '<(engine_library_type)',
    },
    


  ] # end targets

}
