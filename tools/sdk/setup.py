'''
Cozmo, by Anki.

Cozmo is a small robot with a huge personality.

This library lets you take command of Cozmo and write programs for him.

Cozmo features:

    * A camera with advanced vision system
    * Face recognition
    * A robotic lifter
    * Independent tank tracks
    * Pivotable head
    * Cliff detection
    * An array of LEDs
    * An accelerometer
    * A Gyroscope
    * Path planning
    * Animation and behavior systems
    * Power cubes, with LEDs, an accelerometer and tap detection

This SDK provides easy access to take control of Cozmo and write simple
or advanced programs with him.

Requirements:
    * Python 3.5.1 or later

Optional requirements for camera image processing/display:
    * Tkinter (Usually supplied by default with Python)
    * Pillow
    * Numpy
'''


from setuptools import setup, find_packages
import os.path
import sys

if sys.version_info < (3,5,1):
    sys.exit('cozmo requires Python 3.5.1 or later')

here = os.path.abspath(os.path.dirname(__file__))

def fetch_version():
    try:
        with open(os.path.join(here, 'src', 'cozmo', 'version.py')) as f:
            ns = {}
            exec(f.read(), ns)
            return ns['__version__']
    except (IOError, KeyError):
        return 'local'


setup(
    name='cozmo',
    version=fetch_version(),
    description='SDK for Anki Cozmo, the small robot with the big personality',
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
    keywords='anki cozmo robot robotics sdk'.split(),
    package_dir={'': 'src'},
    packages=find_packages('src'),
    package_data={
        'cozmo': ['LICENSE.txt']
    },
    install_requires=None,
    extras_require={
        'camera': ['Pillow>=3.3', 'numpy>=1.11'],
        'test': ['tox', 'pytest'],
    }
)
