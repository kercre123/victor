{
  'variables': {

    'util_source_file_name': 'util.lst',
    'utilunittest_source_file_name': 'utilUnitTest.lst',
    'jsoncpp_source_file_name': 'jsoncpp.lst',
    'kazmath_source_file_name': 'kazmath.lst',
    'audioutil_source_file_name': 'audioUtil.lst',
    'civetweb_source_file_name': 'civetweb.lst',
    'lua_source_file_name': 'lua.lst',
    'folly_source_file_name': 'folly.lst',
    'libwebp_source_file_name': 'libwebp.lst',
    'cpufeatures_source_file_name': 'cpufeatures.lst',
    'networkApp_source_file_name': 'networkApp.lst',
    'generated_clad_source_file_name': 'messages.lst',

    'build_flavor%': 'dev',
    'clad_dir%': '../../tools/message-buffers',
    'has_shipping%': 1,
    'use_libwebp%': 1,
    'use_lua%': 0,
    'use_civetweb%': 0,
    'civetweb_library_type%': 'static_library',
    'lua_library_type%': 'static_library',
    'util_unittest_library_type%': 'shared_library',

    'compiler_flags': [
      '-DJSONCPP_USING_SECURE_MEMORY=0',
      '-DWEBP_RESCALER_FIX=0',
      '-DWEBP_USE_THREAD=1',
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
      ['OS=="linux"', {
        'target_archs%': ['x64'],
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
          '-Wa,--noexecstack',
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
            '-lGLESv2',
            '-llog',
        ],
      },
      { # not android
        'SHARED_LIB_DIR': '', #bogus, just to make the mac / ios builds happy
      },
      ],
      ['OS=="ios" or OS=="mac"', {
        'compiler_flags': [
        '-fobjc-arc',
        ]
      }],
      ['OS=="ios" or OS=="mac" or OS=="linux"', {
        'linker_flags': [
          '-std=c++14',
          '-stdlib=libc++',
          '-lpthread',
        ]
      }]
    ],
  },

  'target_defaults': {
    'cflags': ['<@(compiler_c_flags)'],
    'cflags_cc': ['<@(compiler_cpp_flags)'],
    'ldflags': ['<@(linker_flags)'],
    'include_dirs': [
      '<(clad_dir)/support/cpp/include',
    ],
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'FRAMEWORK_SEARCH_PATHS':'../../libs/framework/',
      'CLANG_CXX_LANGUAGE_STANDARD':'c++14',
      'CLANG_CXX_LIBRARY':'libc++',
      'DEBUG_INFORMATION_FORMAT': 'dwarf',
      'GCC_DEBUGGING_SYMBOLS': 'full',
      'GENERATE_MASTER_OBJECT_FILE': 'YES',
      'ENABLE_BITCODE': 'NO',
      'SKIP_INSTALL': 'YES',
    },
    'target_conditions': [
    ['OS!="cmake"', {
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
            'conditions': [
            [
              "build_flavor=='production' or build_flavor=='prod' or build_flavor=='release'",
              {
                'defines': [
                  'SHIPPING=1',
                ],
              },
            ]
          ],
        },
      }},
      {
        'configurations': {
          '.': {}
        }
      }
    ],
    ['<(has_shipping)==1', {
      'configurations': {
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
      }
    }
    ]],
  },

  'conditions': [    
    [
      "OS=='android'",
      {
        'defines': [
          'ANDROID=1',
        ],

        'targets': [
          {
            'target_name': 'cpufeatures',
            'sources': [ '<!@(cat <(cpufeatures_source_file_name))' ],
            'include_dirs': [
              '../../source/cpufeatures',
            ],
            'direct_dependent_settings': {
              'include_dirs': [
              ],
            },
            'cflags': [
              '-std=c11',
              '-mfpu=neon',
              '-Wno-conditional-uninitialized',
              '-Wno-implicit-function-declaration',
              '-Wno-shorten-64-to-32'
            ],
            'cflags_cc': [
              '-std=c++14',
              '-stdlib=libc++',
              '-mfpu=neon',
              '-Wno-conditional-uninitialized',
              '-Wno-implicit-function-declaration',
              '-Wno-shorten-64-to-32'
            ],
            'type': '<(cpufeatures_library_type)',
          },
        ]
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
      "OS=='linux'",
      {
        'target_defaults': {
          'cflags': [
            '-Wno-format',
            '-Wno-delete-incomplete',
            '-Wno-tautological-constant-out-of-range-compare',
            '-Wno-literal-conversion',
            '-Wno-overloaded-virtual',
            '-Wno-inconsistent-missing-override',
            '-Wa,--noexecstack',
            '-DLINUX=1',
            '-fvisibility=default',
            '-fPIC',
            '-v',
          ],
          'cflags_cc': [
            '-Wno-format',
            '-Wno-delete-incomplete',
            '-Wno-tautological-constant-out-of-range-compare',
            '-Wno-literal-conversion',
            '-Wno-overloaded-virtual',
            '-Wno-inconsistent-missing-override',
            '-Wa,--noexecstack',
            '-DLINUX=1',
            '-fvisibility=default',
            '-fPIC',
            '-v',
          ],
        },
        'targets': [
          {
            'target_name': 'util_UnitTest',
            'sources': [
              "<!@(cat <(util_source_file_name))",
              '<(clad_dir)/support/cpp/source/SafeMessageBuffer.cpp',
            ],
            'conditions': [
              ['OS!="mac"',     {'sources/': [['exclude', '_osx\\.']]}],
              ['OS!="ios"',     {'sources/': [['exclude', '_ios\\.|_iOS\\.']]}],
              ['OS!="android"', {'sources/': [['exclude', '_android\\.']]}],
              ['OS!="linux"',   {'sources/': [['exclude', '_linux\\.']]}],
            ],
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
              '../../source/3rd',
              '../../libs/framework/gtest-linux/include',
              '<(clad_dir)/support/cpp/include',
            ],
            'direct_dependent_settings': {
              'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
              '../../source/3rd',
              '../../libs/framework/gtest-linux/include',
              '<(clad_dir)/support/cpp/include',
              ],
            },
            'defines': [
              'UNIT_TEST=1',
            ],
            'export_dependent_settings': [
              'jsoncpp',
              'kazmath',
              'libwebp',
              'civetweb',
              'lua',
              'cladutil',
            ],
            'dependencies': [
              'jsoncpp',
              'kazmath',
              'libwebp',
              'civetweb',
              'lua',
              'cladutil',
            ],
            'type': '<(util_library_type)',
          },


          {
            'target_name': 'UtilUnitTest',
            'type': 'executable',
            'variables': {
              'linux_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
              '../../libs/framework/gtest-linux/include',
            ],
            'libraries': [
              '../../../../libs/framework/gtest-linux/lib/libgtest.a',
            ],
            'defines': [
              'UNIT_TEST=1',
            ],
            'dependencies': [
              'util_UnitTest',
              'jsoncpp'
            ],
            'sources': [ '<!@(cat <(utilunittest_source_file_name))' ],
          }, # end unittest target

          {
            'target_name': 'networkApp',
            'type': 'executable',
            'variables': {
              'linux_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
            ],
            'defines': [],
            'dependencies': [
              'util_UnitTest'
            ],
            'sources': [ '<!@(cat <(networkApp_source_file_name))' ],
          }, # end networkApp target

        ], # end targets
      },
    ], # end if linux
    [
      "OS=='mac'",
      {
        'variables': {
          'macosx_deployment_target%': '',
        },
        'xcode_settings': {
          'MACOSX_DEPLOYMENT_TARGET' : '<(macosx_deployment_target)',
        },
        'target_defaults': {
          'variables': {
            'mac_target_archs%': [ '<@(target_archs)' ]
          },
          'xcode_settings': {
            'ARCHS': [ '>@(mac_target_archs)' ],
            'SDKROOT': 'macosx',
          },
        },


        'targets': [

          {
            'target_name': 'util_UnitTest',
            'sources': [ 
              "<!@(cat <(util_source_file_name))",
              '<(clad_dir)/support/cpp/source/SafeMessageBuffer.cpp',
            ],
            'conditions': [
              ['OS!="mac"',     {'sources/': [['exclude', '_osx\\.']]}],
              ['OS!="ios"',     {'sources/': [['exclude', '_ios\\.|_iOS\\.']]}],
              ['OS!="android"', {'sources/': [['exclude', '_android\\.']]}],
              ['OS!="linux"',   {'sources/': [['exclude', '_linux\\.']]}],
            ],
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
              '../../source/3rd',
              '<(clad_dir)/support/cpp/include',
            ],
            'direct_dependent_settings': {
              'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
              '../../source/3rd',
              '<(clad_dir)/support/cpp/include',
              ],
            },
            'export_dependent_settings': [
              'jsoncpp',
              'kazmath',
              'libwebp',
              'civetweb',
              'lua',
              'cladutil',
            ],
            'defines': [
              'UNIT_TEST=1',
            ],
            'dependencies': [
              'jsoncpp',
              'kazmath',
              'libwebp',
              'civetweb',
              'lua',
              'cladutil',
            ],
            'xcode_settings': {
              'LD_DYLIB_INSTALL_NAME': '@rpath/$(EXECUTABLE_PATH)'
            },
            'type': '<(util_unittest_library_type)',
          },


          {
            'target_name': 'UtilUnitTest',
            'type': 'executable',
            'variables': {
              'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'xcode_settings': {
              'LD_RUNPATH_SEARCH_PATHS': '@loader_path',
            },
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
            ],
            'defines': [
              'UNIT_TEST=1',
            ],
            'dependencies': [
              'util_UnitTest',
              'jsoncpp'
            ],
            'sources': [ '<!@(cat <(utilunittest_source_file_name))' ],
            'libraries': [
              '../../libs/framework/gtest.framework',
            ],
            'copies': [
              {
                'files': [
                  '../../libs/framework/gtest.framework',
                ],
                'destination': '<(PRODUCT_DIR)',
              },
            ],
          }, # end unittest target

          {
            'target_name': 'networkApp',
            'type': 'executable',
            'variables': {
              'mac_target_archs': [ '$(ARCHS_STANDARD)' ]
            },
            'xcode_settings': {
            },
            'include_dirs': [
              '../../source/anki',
              '../../libs/packaged/include',
            ],
            'defines': [],
            'dependencies': [
              'util_UnitTest'
            ],
            'sources': [ '<!@(cat <(networkApp_source_file_name))' ],
            'libraries': [
              '../../libs/framework/gtest.framework',
            ],
            'copies': [
              {
                'files': [
                  '../../libs/framework/gtest.framework',
                ],
                'destination': '<(PRODUCT_DIR)',
              },
            ],
          }, # end networkApp target

        ], # end targets
      },
    ] # end if mac

  ], #end conditions


  'targets': [
    {
      'target_name': 'libwebp',
      'sources': [ '<!@(cat <(libwebp_source_file_name))' ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/3rd/libwebp/src',
        ],
      },
      'cflags': [
        '-std=c11',
        '-Wno-conditional-uninitialized',
        '-Wno-shorten-64-to-32'
      ],
      'cflags_cc': [
        '-std=c++14',
        '-stdlib=libc++',
        '-Wno-conditional-uninitialized',
        '-Wno-shorten-64-to-32'
      ],
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-std=c11',
          '-Wno-conditional-uninitialized',
          '-Wno-shorten-64-to-32'
        ],
      },
      'conditions': [
        ['OS=="android"',
          {
            'dependencies': [
              'cpufeatures',
            ],
            'include_dirs': [
              '../../source/3rd/libwebp',
              '../../source/3rd/cpufeatures',
            ],
          },
          { # else OS!=android
            'include_dirs': [
              '../../source/3rd/libwebp',
            ],
          }
        ], # endif OS=android
        ['OS!="linux"',
          {
            'cflags': [
              '-mfpu=neon'
            ],
            'cflags_cc': [
              '-mfpu=neon'
            ]
          }
        ] # endif OS!=linux
      ],
      'type': '<(libwebp_library_type)',
    },

    {
      'target_name': 'kazmath',
      'sources': [ '<!@(cat <(kazmath_source_file_name))' ],
      'include_dirs': [
        '../../source/anki',
        '../../libs/packaged/include',
        '../../source/3rd',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/3rd/',
        ],
      },
      'type': '<(kazmath_library_type)',
    },

    {
      'target_name': 'jsoncpp',
      'sources': [ '<!@(cat <(jsoncpp_source_file_name))' ],
      'include_dirs': [
        '../../source/3rd/jsoncpp',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/3rd/jsoncpp',
        ],
      },
      'type': '<(jsoncpp_library_type)',
    },

    {
      'target_name': 'audioUtil',
      'sources': [ '<!@(cat <(audioutil_source_file_name))' ],
      'include_dirs': [
        '../../source/anki/audioUtil',
      ],
      'dependencies': [
        'util'
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/anki/audioUtil',
        ],
      },
      'type': '<(audioutil_library_type)',
      'conditions': [
        ['OS!="mac"',               {'sources/': [['exclude', '_osx\\.']]}],
        ['OS!="ios"',               {'sources/': [['exclude', '_ios\\.|_iOS\\.']]}],
        ['OS!="ios" and OS!="mac"', {'sources/': [['exclude', '_iososx\\.|_osxios\\.']]}],
        ['OS!="android"',           {'sources/': [['exclude', '_android\\.']]}],
        ['OS!="linux"',             {'sources/': [['exclude', '_linux\\.']]}],
        ['OS=="ios" or OS=="mac"',
          {
            'libraries': [
              '$(SDKROOT)/System/Library/Frameworks/CoreFoundation.framework',
              '$(SDKROOT)/System/Library/Frameworks/AudioToolbox.framework'
            ],
          }
        ],
        ['OS=="ios"', { 'libraries': [ '$(SDKROOT)/System/Library/Frameworks/AVFoundation.framework' ] } ],
      ],
    },

    {
      'target_name': 'civetweb',
      'sources': [ '<!@(cat <(civetweb_source_file_name))' ],
      'include_dirs': [
        '../../source/3rd/civetweb/include',
        '../../source/3rd/civetweb/src',
      ],
      'defines': [
        'USE_WEBSOCKET=1',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/3rd/civetweb/include',
          '../../source/3rd/civetweb/src',
        ],
      },
      'type': '<(civetweb_library_type)',
      'cflags': [
        '-Wno-unknown-pragmas',
        '-Wno-shorten-64-to-32'
      ],
      'cflags_cc': [
        '-Wno-unknown-pragmas',
        '-Wno-shorten-64-to-32'
      ],
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-Wno-unknown-pragmas',
          '-Wno-shorten-64-to-32'
        ],
      },
    },

    {
      'target_name': 'lua',
      'sources': [ '<!@(cat <(lua_source_file_name))' ],
      'include_dirs': [
        '../../source/3rd/lua/src',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/3rd/lua/src',
        ],
      },
      'type': '<(lua_library_type)',
      'cflags': [
        '-Wno-unknown-pragmas',
        '-Wno-shorten-64-to-32',
        '-Wno-uninitialized',
        '-Wno-conditional-uninitialized',
        '-Wno-deprecated-declarations',
      ],
      'cflags_cc': [
        '-Wno-unknown-pragmas',
        '-Wno-shorten-64-to-32',
        '-Wno-uninitialized',
        '-Wno-conditional-uninitialized',
        '-Wno-deprecated-declarations',
      ], 
      'xcode_settings': {
        'OTHER_CFLAGS': [
          '-Wno-unknown-pragmas',
          '-Wno-shorten-64-to-32',
          '-Wno-uninitialized',
          '-Wno-conditional-uninitialized',
          '-Wno-deprecated-declarations',
        ],
      },
    },

    {
      'target_name': 'cladutil',
      'sources': [
        '<!@(cat <(generated_clad_source_file_name))',
      ],
      'include_dirs': [
        '../../source/anki/messages',
      ],
      'dependencies': [
        'jsoncpp',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/anki/messages',
        ],
      },
      'type': 'static_library',
    },
    {
      'target_name': 'util',
      'sources': [
        '<!@(cat <(util_source_file_name))',
        '<!@(cat <(folly_source_file_name))',
        '<(clad_dir)/support/cpp/source/SafeMessageBuffer.cpp',
      ],
      'conditions': [
        ['OS!="mac"',     {'sources/': [['exclude', '_osx\\.']]}],
        ['OS!="ios"',     {'sources/': [['exclude', '_ios\\.|_iOS\\.']]}],
        ['OS!="android"', {'sources/': [['exclude', '_android\\.']]}],
        ['OS!="linux"',   {'sources/': [['exclude', '_linux\\.']]}],
        ['<(use_libwebp)==1', {
          'export_dependent_settings': [
            'libwebp'
          ],
          'dependencies': [
            'libwebp'
          ]
        }],
        ['<(use_lua)==1', {
          'export_dependent_settings': [
            'lua'
          ],
          'dependencies': [
            'lua'
          ]
        }],
        ['<(use_civetweb)==1', {
          'export_dependent_settings': [
            'civetweb'
          ],
          'dependencies': [
            'civetweb'
          ]
        }],
        ['OS=="mac"', { 'libraries': [ '$(SDKROOT)/System/Library/Frameworks/Foundation.framework' ] }]
      ],
      'include_dirs': [
        '../../source/anki',
        '../../libs/packaged/include',
        '../../source/3rd',
        '<(clad_dir)/support/cpp/include',
      ],
      'direct_dependent_settings': {
        'include_dirs': [
          '../../source/anki',
          '../../libs/packaged/include',
          '../../source/3rd',
          '<(clad_dir)/support/cpp/include',
        ],
      },
      'export_dependent_settings': [
        'jsoncpp',
        'kazmath',
        'cladutil',
       ],
      'dependencies': [
        'jsoncpp',
        'kazmath',
        'cladutil',
      ],
      'type': '<(util_library_type)',
    },

  ] # end targets

}
