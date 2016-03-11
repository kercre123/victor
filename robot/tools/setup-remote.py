from distutils.core import setup
import sys, os, shutil
import py2exe

BUILD_DIR = "dist"
STRING_TABLE_FILE = os.path.join("..", "resources", "config", "basestation", "AnkiLogStringTables.json")
CLAD_DIR = os.path.join("generated", "cladPython")

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


if not os.path.isdir(CLAD_DIR):
    sys.exit("Please generate robot cladPython before trying to destribute")
if not os.path.isfile(STRING_TABLE_FILE):
    sys.exit("Please generate the string table before trying to destribute")
else:
    shutil.copy2(STRING_TABLE_FILE, BUILD_DIR)
shutil.copytree("releases", os.path.join(BUILD_DIR, "releases"))

shutil.make_archive("remote-dist", "zip", BUILD_DIR, verbose=1)
