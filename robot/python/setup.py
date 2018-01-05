from distutils.core import setup, Extension

module1 = Extension('spine',
                    sources = ['spinemodule.c',
                               'spine_hal.c',
                               'spine_crc.c'])

setup (name = 'SpineHal',
       version = '1.0',
       description = 'This is a spine package',
       ext_modules = [module1])
