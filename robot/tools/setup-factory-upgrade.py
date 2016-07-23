from distutils.core import setup
import sys, os, shutil
import py2exe

BUILD_DIR = "dist"
DIST_PKG = "factory-upgrade-dist.zip"
STRING_TABLE_FILE = os.path.join("..", "resources", "config", "basestation", "AnkiLogStringTables.json")
CLAD_DIR = os.path.join("generated", "cladPython")

try:
    shutil.rmtree(BUILD_DIR)
except:
    sys.stderr.write("Couldn't delete old dist directory, you may need to remove \"{}\" yourself{linesep}".format(BUILD_DIR, linesep=os.linesep))
try:
    if os.path.isfile(DIST_PKG): 
        os.remove(DIST_PKG)
except:
    sys.stderr.write("Couldn't remove old dist zip. You need to remove \"{}\" yourself{linesep}".format(DIST_PKG, linesep))
    
setup(name="factory_upgrade",
      version="1.0",
      description="Factory Restore image upgrader",
      author="Daniel Casner",
      author_email="daniel@anki.com",
      console=[os.path.join('tools', 'fota.py')],
      packages = ['ReliableTransport', 'clad', 'clad.robotInterface', 'clad.types', 'msgbuffers'],
      package_dir = {'ReliableTransport': os.path.join("tools", "ReliableTransport"),
                     'clad': os.path.join("generated", "cladPython", "robot", "clad"),
                     'msgbuffers': os.path.join("..","tools","message-buffers","support","python","msgbuffers")},
      )


if not os.path.isdir(CLAD_DIR):
    sys.exit("Please generate robot cladPython before trying to destribute")
if not os.path.isfile(STRING_TABLE_FILE):
    sys.exit("Please generate the string table before trying to destribute")
else:
    shutil.copy2(STRING_TABLE_FILE, BUILD_DIR)
if not os.path.isdir(os.path.join(BUILD_DIR, "releases")):
    os.mkdir(os.path.join(BUILD_DIR, "releases"))
OTA_SAFE_FILE = os.path.join(BUILD_DIR, "releases", "cozmo.safe")
if os.path.isfile(OTA_SAFE_FILE):
    os.remove(OTA_SAFE_FILE)
shutil.copyfile(os.path.join("staging", "factory_upgrade.safe"), OTA_SAFE_FILE)

base, ext = os.path.splitext(DIST_PKG)
shutil.make_archive(base, ext[1:], BUILD_DIR, verbose=1)
