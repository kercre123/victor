from distutils.core import setup
import os
import py2exe

setup(name="Remote",
      version="1.0",
      description="Factory test remote control app",
      author="Daniel Casner",
      author_email="daniel@anki.com",
      console=[os.path.join('tools', 'remote.py')],
      packages = ['ReliableTransport', 'clad', 'clad.robotInterface', 'clad.types', 'msgbuffers'],
      package_dir = {'ReliableTransport': os.path.join("tools", "ReliableTransport"),
                     'clad': os.path.join("generated", "cladPython", "robot", "clad"),
                     'msgbuffers': os.path.join("..","tools","anki-util","tools","message-buffers","support","python","msgbuffers")},
      )
