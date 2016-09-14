{
  'variables': {

    'if-debug': '-D_LIBCPP_DEBUG=0',
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
            '-landroid',
            '-llog',
        ],
        #'include_directories': [
        #  '../android',
        #  '../android/DASNativeLib/jni',
        #],
      },
      ],
      [ 'OS=="ios"', {
        'compiler_flags': [
            '-fobjc-arc',
        ],
        'linker_flags': [
          '-std=c++11',
          '-stdlib=libc++',
          '-lpthread',
        ],
        #'include_directories': [
        #  '../ios',
        #  '../ios/Libs/Darwin/include',
        #]
      }],
      [ 'OS=="mac"', {
        'compiler_flags': [
            '-fobjc-arc',
        ],
        'linker_flags': [
          '-std=c++11',
          '-stdlib=libc++',
          '-lpthread',
        ],
        #'include_directories': [
        #  '../unix',
        #]
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
    },
    'defines': [
      'USE_DAS=1',
    ],
    'include_dirs': [
        '../../../../tools/anki-util/source/anki'
    ],
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
    'conditions': [
        ['OS=="android"',
          {
            'defines': [
              "ANDROID=1",
            ],
            'ldflags': [
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
            ],
          },
        ],
     ],
  },
  'targets': [
    {
      'target_name': 'BLECozmo',
      'type': 'static_library',
      'include_dirs': [
        '../../shared/',
        '../../ios/',
        '../../ios/Central/',
        '../../ios/Logging/',
      ],
      'sources!': [
        '../../ios/Central/TestBLECentralMultiplexer.h',
        '../../ios/Central/TestBLECentralMultiplexer.m',
      ],
      'link_settings': {
        'libraries': [
          #'libxml2',
        ],
      },
      'direct_dependent_settings': {
        'include_dirs': [
          '../../shared/',
          '../../ios/',
          '../../ios/Central/',
          '../../ios/Logging/',
        ],
        'link_settings': {
          'libraries': [
            #'libxml2',
          ],
        },
      },
      'conditions': [
        ['OS=="ios"', {
          'xcode_settings': {
            'SDKROOT': 'iphoneos',
          },
          'sources': [
            '<!@(cat BLECozmo_ios.lst)',
          ],
        }],
        ['OS=="mac"', {
          'sources': [
            '<!@(cat BLECozmo_ios.lst)',
          ],
        }],
        ['OS=="android"', {
          'sources': [
            '<!@(cat BLECozmo_android.lst)',
          ],
        }],
      ],
    },
  ],
}