# Copyright (c) 2016 Anki, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License in the file LICENSE.txt or at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

'''
Cozmo, by Anki.

Cozmo is a small robot with a big personality.

This library provides a low-level protocol library used by the
cozmo SDK package.
'''


from setuptools import setup, find_packages
import os.path
import sys

if sys.version_info < (3,5,1):
    sys.exit('cozmo requires Python 3.5.1 or later')

here = os.path.abspath(os.path.dirname(__file__))

def fetch_version():
    try:
        with open(os.path.join(here, 'src', 'cozmoclad', '__init__.py')) as f:
            ns = {}
            exec(f.read(), ns)
            return ns['__version__']
    except (IOError, KeyError):
        return 'local'


setup(
    name='cozmoclad',
    version=fetch_version(),
    description='Low-level protocol for the Anki Cozmo SDK.',
    long_description=__doc__,
    url='https://developer.anki.com/cozmo/',
    author='Anki, Inc',
    author_email='cozmosdk@anki.com',
    license='Apache License, Version 2.0',
    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 4 - Beta',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: OSI Approved :: Apache Software License',
        'Programming Language :: Python :: 3.5',
    ],
    zip_safe=True,
    keywords=[],
    package_dir={'': 'src'},
    packages=find_packages('src'),
    package_data={
        'cozmoclad': ['LICENSE.txt']
    },
    install_requires=None,
)
