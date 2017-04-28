{
  'variables': {
    'macosx_deployment_target': '10.11',
    'compiler_flags': [
      '-DALLOW_DEBUG_LOGGING=1'
    ]
  },
  'target_defaults': {
    'target_conditions': [
      ['OS=="ios"', {
        'configurations': {
          'Debug': {
            'xcode_settings': {
              'IPHONEOS_DEPLOYMENT_TARGET': '8.0',
            },
          },
          'Profile': {
            'xcode_settings': {
              'IPHONEOS_DEPLOYMENT_TARGET': '8.0',
            },
          },
          'Release': {
            'xcode_settings': {
              'IPHONEOS_DEPLOYMENT_TARGET': '8.0',
            },
          },
          'Shipping': {
            'xcode_settings': {
              'IPHONEOS_DEPLOYMENT_TARGET': '9.0',
            },
          },
        },
      }],
    ],
  },
}
