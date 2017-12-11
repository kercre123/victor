{
  'variables': {
    # source file lists here...
    'das_source_file_name': 'DAS.lst',
    'das_ios_source_file_name': 'DAS_ios.lst',
    'das_android_source_file_name': 'DAS_android.lst',
    'das_unix_source_file_name': 'DAS_unix.lst',

    'local_cpp_features': 'LOCAL_CPP_FEATURES += rtti exceptions',

    'compiler_flags': [
      '-fdiagnostics-show-category=name',
      '-Wall',
      '-Woverloaded-virtual',
      '-Werror',
      '-fsigned-char',
      '-fvisibility-inlines-hidden',
      '-fvisibility=hidden',
      '-Wshorten-64-to-32',
      '-Winit-self',
      '-Wconditional-uninitialized',
      '-Wno-deprecated-register',
      '-Wformat',
      '-Werror=format-security',
      '-g',
    ],
    'compiler_c_flags' : [
      '-std=c11',
      '<@(compiler_flags)',
    ],
    'compiler_cpp_flags' : [
      '-std=c++14',
      '-stdlib=libc++',
      '<@(compiler_flags)'
    ],
    'linker_flags' : [
        '-g',
    ],
    'include_directories': [
        '../include',
        '../include/DAS',
        '../src',
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
          '--sysroot=<(ndk_root)/platforms/<(android_platform)/arch-arm',
          '-DANDROID=1',
          '-DNO_LOCALE_SUPPORT=1',
          '-gcc-toolchain', '<(ndk_root)/toolchains/<(android_toolchain)/prebuilt/darwin-x86_64',
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
          '-I<(ndk_root)/platforms/<(android_platform)/arch-arm/usr/include',
        ],
        'linker_flags': [
            '--sysroot=<(ndk_root)/platforms/<(android_platform)/arch-arm',
            '-gcc-toolchain', '<(ndk_root)/toolchains/<(android_toolchain)/prebuilt/darwin-x86_64',
            '-no-canonical-prefixes',
            '-target armv7-none-linux-androideabi',
            '-Wl,--fix-cortex-a8',
            '-Wl,--no-undefined',
            '-Wl,-z,noexecstack',
            '-Wl,-z,relro',
            '-Wl,-z,now',
            '-mthumb',
            '-L<(ndk_root)/platforms/<(android_platform)/arch-arm/usr/lib',
            '-L<(ndk_root)/sources/cxx-stl/llvm-libc++/libs/armeabi-v7a',
            '-lgcc',
            '-lc',
            '-lm',
            '-lc++_shared',
            '-landroid',
            '-llog',
        ],
        'include_directories': [
          '../android',
          '../android/DASNativeLib/jni',
        ],
      },
      ],
      [ 'OS=="ios"', {
        'compiler_flags': [
            '-fobjc-arc',
        ],
        'linker_flags': [
          '-std=c++14',
          '-stdlib=libc++',
          '-lpthread',
        ],
        'include_directories': [
          '../ios',
          '../ios/Libs/Darwin/include',
        ]
      }],
      [ 'OS=="mac"', {
        'compiler_flags': [
            '-fobjc-arc',
        ],
        'linker_flags': [
          '-std=c++14',
          '-stdlib=libc++',
          '-lpthread',
        ],
        'include_directories': [
          '../unix',
        ]
      }]
    ],
  },

  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'ldflags': ['<@(linker_flags)'],
    'xcode_settings': {
        'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
        'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
        'ALWAYS_SEARCH_USER_PATHS': 'NO',
        'DYLIB_INSTALL_NAME_BASE': '@loader_path',
        'ENABLE_BITCODE': 'NO',
        'SKIP_INSTALL': 'YES',
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
      'Shipping': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'xcode_settings': {
            'OTHER_CFLAGS': ['-Os'],
            'OTHER_CPLUSPLUSFLAGS': ['-Os'],
          },
          'defines': [
            'NDEBUG=1',
            'SHIPPING=1',
          ],
      },
    },
  },

  'conditions': [
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
      "OS=='mac'",
      {
        'variables': {
          'macosx_deployment_target%': '',
        },
        'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET': '<(macosx_deployment_target)',
        },
      },
    ],
  ],

  'targets': [
    {
      'target_name': 'DAS',
      'type': '<(das_library_type)',
      'product_prefix': 'lib',
      'dependencies': [
        #none
      ],
      'include_dirs': [
        '<@(include_directories)',
      ],
      'sources': [
        '<!@(cat <(das_source_file_name))',
      ],
      'direct_dependent_settings': {
        'include_dirs': ['../include'],
      },
      'conditions': [
        ['OS=="ios"', {
          'sources': [
            '<!@(cat <(das_ios_source_file_name))',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            '<!@(cat <(das_unix_source_file_name))',
          ],
        }],
        ['OS=="android"', {
          'sources': [
            '<!@(cat <(das_android_source_file_name))',
          ],
        }],

      ],
    },
  ],

}
