{
  'variables': {

    'engine_source_file_name': 'cozmoEngine.lst',
    
    'compiler_flags': [
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
    'include_dirs': [
      '../../tools/message-buffers/support/cpp/include',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'FRAMEWORK_SEARCH_PATHS':'../../libs/framework/',
      'CLANG_CXX_LANGUAGE_STANDARD':'c++11',
      'CLANG_CXX_LIBRARY':'libc++',
      'DEBUG_INFORMATION_FORMAT': 'dwarf',
      'GCC_DEBUGGING_SYMBOLS': 'full',
      #'GCC_PREFIX_HEADER': '../../source/anki/basestation/basestation.pch',
      #'GCC_PRECOMPILE_PREFIX_HEADER': 'YES',
      'GCC_ENABLE_SYMBOL_SEPARATION': 'YES',
      'GENERATE_MASTER_OBJECT_FILE': 'YES',
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
    [
      "OS=='ios' or OS=='mac'",
      {
          'targets': [
          {
            # fake target to see all of the sources...
            'target_name': 'all_lib_targets',
            'type': 'none',
            'sources': [
              '<!@(cat <(engine_source_file_name))',
            ],
            'dependencies': [
              'cozmoEngine',
            ]
          },
        ],
      },
    ],
    # [
    #   "OS=='mac'",
    #   {
    #     'target_defaults': {
    #       'variables': {
    #         'mac_target_archs%': [ '<@(target_archs)' ]
    #       },
    #       'xcode_settings': {
    #         'ARCHS': [ '>@(mac_target_archs)' ],
    #         'SDKROOT': 'macosx',
    #         'MACOSX_DEPLOYMENT_TARGET': '10.9',
    #       },
    #     },


    #     'targets': [
    #       {
    #         'target_name': 'UnitTest',
    #         'type': 'executable',
    #         'variables': {
    #           'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
    #         },
    #         'xcode_settings': {
    #         },
    #         'include_dirs': [
    #           '../../source/anki',
    #           '../../libs/anki-util/source/anki',
    #         ],
    #         'defines': [
    #           'UNIT_TEST=1',
    #         ],
    #         'dependencies': [
    #           'cozmoEngine',
    #         ],
    #         'sources': [ '<!@(cat <(unittest_source_file_name))' ],
    #         'libraries': [
    #           '../../libs/anki-util/libs/framework/gtest.framework',
    #         ],
    #         'copies': [
    #           {
    #             'files': [
    #               '../../libs/anki-util/libs/framework/gtest.framework',
    #             ],
    #             'destination': '<(PRODUCT_DIR)',
    #           },
    #           {
    #             # this replaces copyUnitTestData.sh
    #             'files': [
    #               '../../resources/config',
    #             ],
    #             'destination': '<(PRODUCT_DIR)/BaseStation',
    #           },
    #         ],
    #       }, # end unittest target



    #     ], # end targets
    #   },
    # ] # end if mac


  ], #end conditions


  'targets': [

    {
      'target_name': 'cozmoEngine',
      'sources': [ "<!@(cat <(engine_source_file_name))" ],
      'include_dirs': [
        '../../basestation/src',
        '../../basestation/include',
      ],
      'target_conditions': [
        [
          "OS=='mac'",
          {
            'xcode_settings': {
              'LD_RUNPATH_SEARCH_PATHS': '@loader_path',
              'LD_DYLIB_INSTALL_NAME': '@loader_path/$(EXECUTABLE_PATH)'
            },
          } #end not android
        ],
      ],
      # 'dependencies': [
      #   'worldviz',
      #   '../../libs/anki-util/project/gyp/util.gyp:util',
      #   '../../libs/anki-util/project/gyp/util.gyp:jsoncpp',
      #   '../../libs/anki-util/project/gyp/util.gyp:kazmath'        
      # ],
      'type': '<(energy_library_type)',
    },
    

    # # Post-build target to strip Android libs
    # {
    #   'target_name': 'strip_android_libs',
    #   'type': 'none',
    #   'dependencies': [
    #     '<(driveengine_target_name)',
    #     '<(basestation_target_name)',
    #   ],
    #   'target_conditions': [
    #     [
    #       "OS=='android' and use_das==1",
    #       {
    #         'actions': [
    #           {
    #             'action_name': 'strip_debug_symbols',
    #             'inputs': [
    #               '<(SHARED_LIB_DIR)/lib<(driveengine_target_name).so',
    #               '<(SHARED_LIB_DIR)/libDriveAudioEngine.so',
    #               '<(SHARED_LIB_DIR)/libDAS.so',
    #               '<(SHARED_LIB_DIR)/libc++_shared.so',
    #             ],
    #             'outputs': [
    #               '<(SHARED_LIB_DIR)/../libs-debug/lib<(driveengine_target_name).so',
    #               '<(SHARED_LIB_DIR)/../libs-debug/libDriveAudioEngine.so',
    #               '<(SHARED_LIB_DIR)/../libs-debug/libDAS.so',
    #               '<(SHARED_LIB_DIR)/../libs-debug/libc++_shared.so',
    #             ],
    #             'action': [
    #               'python',
    #               'android-strip-libs.py',
    #               '--ndk-toolchain',
    #               '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
    #               '--symbolicated-libs-dir',
    #               '<(SHARED_LIB_DIR)/../lib-debug',
    #               '<@(_inputs)',
    #             ],
    #           },
    #         ],
    #       },
    #     ], #end android
    #     [
    #       "OS=='android' and use_das==0",
    #       {
    #         'actions': [
    #           {
    #             'action_name': 'strip_debug_symbols',
    #             'inputs': [
    #               '<(SHARED_LIB_DIR)/lib<(driveengine_target_name).so',
    #               '<(SHARED_LIB_DIR)/libDriveAudioEngine.so',
    #               '<(SHARED_LIB_DIR)/libc++_shared.so',
    #             ],
    #             'outputs': [
    #               '<(SHARED_LIB_DIR)/../libs-debug/lib<(driveengine_target_name).so',
    #               '<(SHARED_LIB_DIR)/../libs-debug/libDriveAudioEngine.so',
    #               '<(SHARED_LIB_DIR)/../libs-debug/libc++_shared.so',
    #             ],
    #             'action': [
    #               'python',
    #               'android-strip-libs.py',
    #               '--ndk-toolchain',
    #               '<(ndk_root)/toolchains/arm-linux-androideabi-4.8/prebuilt/darwin-x86_64',
    #               '--symbolicated-libs-dir',
    #               '<(SHARED_LIB_DIR)/../lib-debug',
    #               '<@(_inputs)',
    #             ],
    #           },
    #         ],
    #       },
    #     ], #end android
    #   ],


    # },


  ] # end targets

}
