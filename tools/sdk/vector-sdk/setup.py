# Copyright (c) 2018 Anki, Inc.

'''
Vector, by Anki.

Requirements:
    * Python 3.5.1 or later
'''

import os.path
import sys
from setuptools import setup

if sys.version_info < (3, 5, 1):
    sys.exit('vector requires Python 3.5.1 or later')

HERE = os.path.abspath(os.path.dirname(__file__))

def fetch_version():
    '''Get the version from the package'''
    with open(os.path.join(HERE, 'vector', 'version.py')) as version_file:
        versions = {}
        exec(version_file.read(), versions)
        return versions

VERSION_DATA = fetch_version()
VERSION = VERSION_DATA['__version__']

setup(
    name='vector',
    version=VERSION,
    description='SDK for Anki Vector',
    long_description=__doc__,
    url='https://developer.anki.com/vector/',
    author='Anki, Inc',
    author_email='vectorsdk@anki.com', # TODO: set up this email
    license='Other/Proprietary License', # TODO: update when we update the license
    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 2 - Pre-Alpha',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: Other/Proprietary License', # TODO: update when we update the license
        'Programming Language :: Python :: 3.5',
    ],
    zip_safe=True,
    keywords='anki vector robot robotics sdk'.split(),
    install_requires=['aiogrpc>=1.4',
                      'docopt',
                      'googleapis-common-protos'],
)
