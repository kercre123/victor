{
  'includes': [
    '../../lib/anki/cozmo-engine/coretech/project/gyp/opencv.gypi',
    '../../lib/anki/cozmo-engine/coretech/project/gyp/face-library.gypi'
  ],

  'variables': {

    'game_source': 'cozmoGame.lst',
    'game_library_type': 'static_library',
    'ctrlGameEngine_source': 'ctrlGameEngine.lst',
    'ctrlKeyboard_source': 'ctrlKeyboard.lst',
    'ctrlBuildServerTest_source': 'ctrlBuildServerTest.lst',    
    'csharp_source': 'csharp.lst',
    'buildMex': '<(build-mex)',
    
    # TODO: should this be passed in, or shared?
    'coretech_defines': [
      'ANKICORETECH_USE_MATLAB=0',
      # 'ANKICORETECH_USE_GTEST=1',
      'ANKICORETECH_USE_OPENCV=1',
      'ANKICORETECH_EMBEDDED_USE_MATLAB=0',
      'ANKICORETECH_EMBEDDED_USE_GTEST=1',
      'ANKICORETECH_EMBEDDED_USE_OPENCV=1',
    ],

    'sphinx_libs': [
      'libpocketSphinx.a',
      'libsphinxad.a',
      'libsphinxBase.a',
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
    ],
    
    'compiler_flags': [
      '-Wdeprecated-declarations',
      '-fdiagnostics-show-category=name',
      '-Wall',
      '-Woverloaded-virtual',
      '-Werror',
      # '-Wundef', # Disabled until define usage is refactored to code standards
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
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_ios_debug)',
                  '<(webots_path)/lib/',
                ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_mac_debug)',
                  '<(webots_path)/lib/',
                ],
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
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_ios_release)',
                  '<(webots_path)/lib/',
                ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_mac_release)',
                  '<(webots_path)/lib/',
                ],
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
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_ios_release)',
                  '<(webots_path)/lib/',
                ],
              },
            }],
            ['OS=="mac"', {
              'xcode_settings': {
                'LIBRARY_SEARCH_PATHS': [
                  '<@(cte_lib_search_path_mac_release)',
                  '<(webots_path)/lib/',
                ],
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
        'targets': [
          {
            'target_name': 'CSharpBinding',
            'type': 'static_library',
            'include_dirs': [
              '../../unit/CSharpBinding/src',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              'cozmoGame',
              '<(cg-ce_gyp_path):cozmoEngine',
              '<(cg-cti_gyp_path):ctiCommon',
              '<(cg-cti_gyp_path):ctiMessaging',
              '<(cg-cti_gyp_path):ctiPlanning',
              '<(cg-cti_gyp_path):ctiVision',
              '<(cg-util_gyp_path):util',
              '<(cg-util_gyp_path):jsoncpp',
            ],
            'sources': [ '<!@(cat <(csharp_source))' ],
            'libraries': [
            ],
          }, # end CSharpBinding

          {
            # fake target to see all of the sources...
            'target_name': 'all_lib_targets',
            'type': 'none',
            'dependencies': [
              'cozmoGame',
              'CSharpBinding',
              '<(cg-ce_gyp_path):cozmoEngine',
              '<(cg-cti_gyp_path):ctiCommon',
              '<(cg-cti_gyp_path):ctiCommonRobot',
              '<(cg-cti_gyp_path):ctiMessaging',
              '<(cg-cti_gyp_path):ctiMessagingRobot',
              '<(cg-cti_gyp_path):ctiPlanning',
              '<(cg-cti_gyp_path):ctiPlanningRobot',
              '<(cg-cti_gyp_path):ctiVision',
              '<(cg-cti_gyp_path):ctiVisionRobot',
              '<(cg-util_gyp_path):util',
              '<(cg-util_gyp_path):jsoncpp',
            ]
          },
        ],
      },
    ],



    # MEX CRAP HERE
    # MEX CRAP HERE
    # MEX CRAP HERE
    # MEX CRAP HERE
    [
      "buildMex=='yes'",
      {
        'targets': [
          {
            'target_name': 'allMexTargets',
            'type': 'none',
            'dependencies': [
              '<(cg-mex_gyp_path):mexDetectFiducialMarkers',
              '<(cg-mex_gyp_path):mexUnique',
              '<(cg-mex_gyp_path):mexCameraCapture',
              '<(cg-mex_gyp_path):mexHist',
              '<(cg-mex_gyp_path):mexClosestIndex',
              '<(cg-mex_gyp_path):mexRegionprops',
              '<(cg-mex_gyp_path):mexRefineQuadrilateral',
            ],
          },
        ],
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
            'target_name': 'webotsCtrlGameEngine',
            'type': 'executable',
            'include_dirs': [
              '<@(webots_includes)',
              '<@(opencv_includes)',
            ],
            'dependencies': [
              'cozmoGame',
              '<(cg-ce_gyp_path):cozmoEngine',
              '<(cg-cti_gyp_path):ctiCommon',
              '<(cg-cti_gyp_path):ctiCommonRobot',
              '<(cg-cti_gyp_path):ctiVision',
              '<(cg-cti_gyp_path):ctiVisionRobot',
              '<(cg-util_gyp_path):util',
              '<(cg-util_gyp_path):jsoncpp',
            ],
            'sources': [ '<!@(cat <(ctrlGameEngine_source))' ],
            'libraries': [
              'libCppController.dylib',
              '$(SDKROOT)/System/Library/Frameworks/Cocoa.framework',
              '$(SDKROOT)/System/Library/Frameworks/AppKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/QTKit.framework',
              '$(SDKROOT)/System/Library/Frameworks/QuartzCore.framework',
              '$(SDKROOT)/System/Library/Frameworks/OpenAL.framework',
              '<@(sphinx_libs)',
              '<@(opencv_libs)',
              '<@(face_library_libs)',
            ],
            'actions': [
              {
                'action_name': 'create_symlink_webotsCtrlEnginefaceLibraryLibs',
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
                      '../../simulator/controllers/webotsCtrlGameEngine/',
                    ],
                  }],
                  ['face_library=="facesdk"', {
                    'action': [
                      'ln',
                      '-s',
                      '-f',
                      '<(face_library_lib_path)/libfsdk.dylib',
                      '../../simulator/controllers/webotsCtrlGameEngine/',
                    ],
                  }],
                ], # conditions
              },
            ] # actions
          }, # end controller Game Engine

          {
            'target_name': 'webotsCtrlKeyboard',
            'type': 'executable',
            'include_dirs': [
              '<@(webots_includes)',
              '<@(opencv_includes)',
              '<(cti-cozmo_engine_path)/simulator/include'
            ],
            'dependencies': [
              'cozmoGame',
              '<(cg-ce_gyp_path):cozmoEngine',
              '<(cg-util_gyp_path):util',
              '<(cg-cti_gyp_path):ctiCommon',
              '<(cg-cti_gyp_path):ctiVision',
              '<(cg-cti_gyp_path):ctiMessaging',
              '<(cg-cti_gyp_path):ctiPlanning',
            ],
            'sources': [ '<!@(cat <(ctrlKeyboard_source))' ],
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
                    'action_name': 'create_symlink_webotsCtrlKeyboard_faciometricLibs',
                    'inputs': [ ],
                    'outputs': [ ],
                    'action': [
                      'ln',
                      '-s',
                      '-h',
                      '-f',
                      '<(face_library_lib_path)',
                      '../../simulator/controllers/webotsCtrlKeyboard/',
                    ],
                  },
                ], # actions
              }], # conditions
            ],
          }, # end controller Keyboard

          {
            'target_name': 'webotsCtrlBuildServerTest',
            'type': 'executable',
            'include_dirs': [
              '<@(webots_includes)',
              '<@(opencv_includes)',
              '<(cti-cozmo_engine_path)/simulator/include',
            ],
            'dependencies': [
              'cozmoGame',
              '<(cg-ce_gyp_path):cozmoEngine',
              '<(cg-util_gyp_path):util',
              '<(cg-cti_gyp_path):ctiCommon',
              '<(cg-cti_gyp_path):ctiVision',
              '<(cg-cti_gyp_path):ctiMessaging',
              '<(cg-cti_gyp_path):ctiPlanning',
            ],
            'sources': [ '<!@(cat <(ctrlBuildServerTest_source))' ],
            'libraries': [
              'libCppController.dylib',
              '<@(opencv_libs)',
            ],
          }, # end controller Keyboard

          {
            'target_name': 'allUnitTests',
            'type': 'none',
            'dependencies': [
              '<(cg-ce_gyp_path):cozmoEngineUnitTest',
              '<(cg-cti_gyp_path):ctiUnitTest',
              '<(cg-util_gyp_path):UtilUnitTest',
            ],
          },

          {
            'target_name': 'webotsControllers',
            'type': 'none',
            'dependencies': [
              'webotsCtrlKeyboard',
              'webotsCtrlBuildServerTest',              
              'webotsCtrlGameEngine',
              '<(cg-ce_gyp_path):webotsCtrlRobot',
              '<(cg-ce_gyp_path):webotsCtrlViz',
              '<(cg-ce_gyp_path):webotsCtrlLightCube',
              '<(cg-ce_gyp_path):cozmo_physics',
            ],
            'copies': [
            
              # Copy over protos
              {
                'files': [
                  '../../lib/anki/cozmo-engine/simulator/protos',
                ],
                'destination': '../../simulator/protos/CozmoEngineProtos-DO_NOT_MODIFY/',
              },

            ],
            
            # Create symlinks to controller binaries
            # For some reason this is necessary in order to be able to attach to their processes from Xcode.
            'actions': [
              {
                'action_name': 'create_symlink_webotsCtrlKeyboard',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlKeyboard',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlKeyboard/webotsCtrlKeyboard',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_webotsCtrlBuildServerTest',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlBuildServerTest',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlBuildServerTest/webotsCtrlBuildServerTest',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },              
              {
                'action_name': 'create_symlink_webotsCtrlGameEngine',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlGameEngine',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlGameEngine/webotsCtrlGameEngine',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_resources_assets',
                'inputs': [
                  '<(cozmo_asset_path)',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlGameEngine/resources/assets',
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
                'action_name': 'create_symlink_resources_configs',
                'inputs': [
                  '<(cozmo_engine_path)/resources/config',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlGameEngine/resources/config',
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
                  '../../simulator/controllers/webotsCtrlGameEngine/resources/test',
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
                  '../../simulator/controllers/webotsCtrlGameEngine/resources/pocketsphinx',
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
                'action_name': 'create_symlink_webotsCtrlRobot',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlRobot',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlRobot/webotsCtrlRobot',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_webotsCtrlViz',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlViz',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlViz/webotsCtrlViz',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_webotsCtrlLightCube',
                'inputs': [
                  '<(PRODUCT_DIR)/webotsCtrlLightCube',
                ],
                'outputs': [
                  '../../simulator/controllers/webotsCtrlLightCube/webotsCtrlLightCube',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              {
                'action_name': 'create_symlink_webotsPluginPhysics',
                'inputs': [
                  '<(PRODUCT_DIR)/libcozmo_physics.dylib',
                ],
                'outputs': [
                  '../../simulator/plugins/physics/cozmo_physics/libcozmo_physics.dylib',
                ],
                'action': [
                  'ln',
                  '-s',
                  '-f',
                  '<@(_inputs)',
                  '<@(_outputs)',
                ],
              },
              
            ], # actions
            
          }, # end webotsControllers

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
      'target_name': 'cozmoGame',
      'sources': [ 
        '<!@(cat <(game_source))',
      ],
      'include_dirs': [
        '../../game/src',
        '../../game/include',
        '../../generated/clad/game',
        '<@(opencv_includes)',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../game/include',
          '../../generated/clad/game',
        ],
      },
      'dependencies': [
        '<(cg-util_gyp_path):util',
        '<(cg-ce_gyp_path):cozmoEngine',
        '<(cg-cti_gyp_path):ctiCommon',
        '<(cg-cti_gyp_path):ctiMessaging',
        '<(cg-cti_gyp_path):ctiPlanning',
        '<(cg-cti_gyp_path):ctiVision',
      ],
      'type': '<(game_library_type)',
      
      'conditions': [
        ['face_library=="faciometric"', {
          # Copy FacioMetric's models into the resources so they are available at runtime.
          # This is a little icky since it reaches into cozmo engine...
          'actions': [
            {
              'action_name': 'copy_faciometric_models',
              'inputs': [
                '<(face_library_path)/Demo/models',
              ],
              'outputs': [
                '../../lib/anki/cozmo-engine/resources/config/basestation/vision/faciometric',
              ],
              'action': [
                'cp',
                '-R',
                '<@(_inputs)',
                '<@(_outputs)',
              ],
            },
          ],
        }],
      ] #'conditions'

    },
    

  ] # end targets

}
