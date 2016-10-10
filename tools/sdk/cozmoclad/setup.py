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
    license='BSD',
    # See https://pypi.python.org/pypi?%3Aaction=list_classifiers
    classifiers=[
        'Development Status :: 3 - Alpha',
        'Intended Audience :: Developers',
        'Topic :: Software Development :: Libraries',
        'License :: OSI Approved :: BSD License',
        'Programming Language :: Python :: 3.5',
    ],
    zip_safe=True,
    keywords=[],
    package_dir={'': 'src'},
    packages=find_packages('src'),
    package_data={
        'cozmosdk': ['LICENSE.txt']
    },
    install_requires=None,
)
