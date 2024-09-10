{
  'variables': {
    'audio_engine_name': 'AudioEngine',
    'source_file_name': 'audioEngine.lst',
    'audio_library_build%': 'profile',

    # Note: This path is for OD, Cozmo project will override these vars
    'util_gyp_path%': '../../anki-util/project/gyp/util.gyp',
    'generated_clad_path%': '../../basestation/source/anki/messages',
    # Note: I'm tired of changing paths every time I need to repoint OD to audio lib repo  - JMR
    #'util_gyp_path%': '../../../../OverDrive/Source/overdrive-one/lib/anki-util/project/gyp/util.gyp',
    #'generated_clad_path%': '../../../../OverDrive/Source/overdrive-one/lib/basestation/source/anki/messages',

    # wwise SDK location
    'wwise_sdk_root%': '../wwise/versions/current',

    'audio_specific_compiler_flags' : [
      '-Wno-deprecated-declarations'
    ],
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
      '-Wa,--noexecstack',
      '-Wformat',
      '-Werror=format-security',
      '-g',
      '-Wno-deprecated-declarations',
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
    'target_archs%': ['$(ARCHS_STANDARD_32_64_BIT)'],

    # Copy overridden vars into this scope
    'conditions': [
      ['OS=="android"', {
        'target_archs%': ['armveabi-v7a'],
        'compiler_flags': [
          '--sysroot=<(ndk_root)/platforms/<(android_platform)/arch-arm',
          '-DANDROID=1',
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
            '-lGLESv2',
        ],
      },
      { # not android
        'SHARED_LIB_DIR': '', #bogus, just to make the mac / ios builds happy
      },
    ],
    ['OS=="ios" or OS=="mac"', {
        'compiler_flags': [
          '-fobjc-arc',
        ],
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
    'xcode_settings': {
      'OTHER_CFLAGS': ['<@(compiler_c_flags)'],
      'OTHER_CPLUSPLUSFLAGS': ['<@(compiler_cpp_flags)'],
      'ALWAYS_SEARCH_USER_PATHS': 'NO',
      'ENABLE_BITCODE': 'NO',
      'SKIP_INSTALL': 'YES',
      'STRIP_STYLE': 'debugging',
    },
    'configurations': {
      'Debug': {
          'cflags': ['-O0'],
          'cflags_cc': ['-O0'],
          'xcode_settings': {
              'GCC_OPTIMIZATION_LEVEL': '0',
              'COPY_PHASE_STRIP': 'NO',
              'STRIP_INSTALLED_PRODUCT': 'NO',
          },
          'defines': [
            'DEBUG=1',
          ],
      },
      'Profile': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
          'defines': [
            'NDEBUG=1',
            'PROFILE=1',
          ],
      },
      'Release': {
          'cflags': ['-Os'],
          'cflags_cc': ['-Os'],
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
        'target_defaults': {
          'defines': [
            'ANDROID=1',
          ],
        }
      },
    ],
    [ 
      "OS=='ios'", 
      {
        'xcode_settings': {
          'SDKROOT': 'iphoneos',
          'VALID_ARCHS': ['<@(target_archs)'],
          #'TARGETED_DEVICE_FAMILY': '1,2',
          #'CODE_SIGN_IDENTITY': 'iPhone Developer',
          'IPHONEOS_DEPLOYMENT_TARGET': '9.0',
        },
        'target_defaults': {
          'defines': [
          
          ]
        }
      },
    ],
    [ 
      "OS=='mac'", 
      {
        'xcode_settings': {
          'ARCHS': ['<@(target_archs)'],
        },
        'target_defaults': {
          'defines': [
          
          ]
        }
      }, 
    ],
    ["audio_library_build=='release'", {
        'target_defaults': {
            'defines': [
                'AK_OPTIMIZED=1'
            ],
        }
    }],
  ], 



  'targets':
  [
    {
      'target_name': 'all_target_libs',
      'type': 'none',
      'dependencies': [
      '<(util_gyp_path):util'
      ]
    },
    {
      'target_name': '<(audio_engine_name)',
      'product_prefix': 'lib',
      'type': '<(audio_library_type)',
      'sources': [
        '<!@(cat <(source_file_name))',
      ],
      'include_dirs':
      [
        '../source',
        '<(wwise_sdk_root)/includes',
        '../include',
        '../include/audioEngine',
        '../plugins/hijackAudio/pluginInclude',
        '../plugins/hijackAudio/pluginSource',
        '../plugins/wavePortal/pluginInclude',
        '../plugins/wavePortal/pluginSource',
        '<(generated_clad_path)',
      ],
      'direct_dependent_settings': {
        'include_dirs':
        [
          '../include',
          '../plugins/hijackAudio/pluginInclude',
          '../plugins/wavePortal/pluginInclude',
        ],
      },
      'dependencies': [
      '<(util_gyp_path):util'
      ],
      'variables': {
        'wwise_libs':
        [
          '-lCommunicationCentral',
          '-lAkStreamMgr',
          '-lAkMusicEngine',
          '-lAkSoundEngine',
          '-lAkMemoryMgr',
          # Audio Kinetic Effect Plugins
          '-lAkCompressorFX',
          '-lAkDelayFX',
          '-lAkMatrixReverbFX',
          '-lAkMeterFX',
          '-lAkExpanderFX',
          '-lAkParametricEQFX',
          '-lAkGainFX',
          '-lAkPeakLimiterFX',
          '-lAkSoundSeedImpactFX',
          '-lAkRoomVerbFX',
          '-lAkGuitarDistortionFX',
          '-lAkStereoDelayFX',
          '-lAkPitchShifterFX',
          '-lAkTimeStretchFX',
          '-lAkFlangerFX',
          '-lAkConvolutionReverbFX',
          '-lAkTremoloFX',
          '-lAkHarmonizerFX',
          '-lAkRecorderFX',
          # McDSP
          '-lMcDSPLimiterFX',
          '-lMcDSPFutzBoxFX',
          # Crankcase REV
          '-lCrankcaseAudioREVModelPlayerFX',
          # Sources plug-ins
          '-lAkSilenceSource',
          '-lAkSineSource',
          '-lAkToneSource',
          '-lAkAudioInputSource',
          '-lAkSoundSeedWoosh',
          '-lAkSoundSeedWind',
          '-lAkSynthOne',
          # Required by codecs plug-ins
          '-lAkVorbisDecoder',
        ],
        'conditions': [
          ["'<(audio_library_build)'=='release'", {'wwise_libs!': ['-lCommunicationCentral']}]
        ],
      },
      'target_conditions':
      [
        [
          "OS=='android'",
          {
            'libraries':
            [
              '<@(wwise_libs)',
              '-llog',
              '-lOpenSLES',
              '-landroid',
            ],
            'library_dirs':
            [
              '<(wwise_sdk_root)/libs/android/<(audio_library_build)',
            ],
            'include_dirs':
            [
              '../zipreader',
            ]
          },
	      ], #end android

        # Non-Android settings ...
        [
          'OS=="mac" or OS=="ios"',
          {
            'libraries':
            [
              'libCommunicationCentral.a',
              'libAkStreamMgr.a',
              'libAkMusicEngine.a',
              'libAkSoundEngine.a',
              'libAkMemoryMgr.a',
              # Audio Kinetic Effect Plugins
              'libAkCompressorFX.a',
              'libAkDelayFX.a',
              'libAkMatrixReverbFX.a',
              'libAkMeterFX.a',
              'libAkExpanderFX.a',
              'libAkParametricEQFX.a',
              'libAkGainFX.a',
              'libAkPeakLimiterFX.a',
              'libAkSoundSeedImpactFX.a',
              'libAkRoomVerbFX.a',
              'libAkGuitarDistortionFX.a',
              'libAkStereoDelayFX.a',
              'libAkPitchShifterFX.a',
              'libAkTimeStretchFX.a',
              'libAkFlangerFX.a',
              'libAkConvolutionReverbFX.a',
              'libAkTremoloFX.a',
              'libAkHarmonizerFX.a',
              'libAkRecorderFX.a',
              # McDSP
              'libMcDSPLimiterFX.a',
              'libMcDSPFutzBoxFX.a',
              # Crankcase REV
              'libCrankcaseAudioREVModelPlayerFX.a',
              # Sources plug-ins
              'libAkSilenceSource.a',
              'libAkSineSource.a',
              'libAkToneSource.a',
              'libAkAudioInputSource.a',
              'libAkSoundSeedWoosh.a',
              'libAkSoundSeedWind.a',
              'libAkSynthOne.a',
              # Required by codecs plug-ins
              'libAkAACDecoder.a',
              'libAkVorbisDecoder.a',
            ],
            'conditions': [
              ["'<(audio_library_build)'=='release'", {'libraries!': ['libCommunicationCentral.a']}]
            ]
          }
        ], #end mac/ios

        [
          "OS=='mac'",
          {
            'library_dirs':
            [
              '<(wwise_sdk_root)/libs/mac/<(audio_library_build)',
            ],
          }
        ], #end mac
        
	      [
	        "OS=='ios'",
          {
            'library_dirs':
            [
              '<(wwise_sdk_root)/libs/ios/<(audio_library_build)',
            ],
          },
        ], #end ios
      ], #end target_conditions
    }
  ]
}
