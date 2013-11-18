#!/usr/bin/python

import os
import sys
import string
import subprocess

def addSources(dir):
  sources = []
  for root, subFolders, files in os.walk(dir):
    for file in files:
      if root == dir:
        f = os.path.join(root, file)
        f = str.replace(f, '\\', '/')
        sources.append(f)
  return sources

TARGET = 'cozmo'

MV_SOC_PLATFORM = 'myriad1'
GCC_VERSION = '4.4.2'
MV_TOOLS_VERSION = '00.50.37.0'

LEON_SOURCE = []
LEON_SOURCE += addSources('./hal')
LEON_SOURCE += addSources('./vision')
#LEON_SOURCE += addSources('./supervisor/src')
LEON_SOURCE += addSources('../coretech/vision/robot/src')
LEON_SOURCE += addSources('../coretech/messaging/robot/src')
LEON_SOURCE += addSources('../coretech/common/robot/src')
LEON_SOURCE += addSources('../coretech/common/shared/src')

SHAVE_SOURCE = []

MV_TOOLS_DIR = str.replace(os.environ.get('MV_TOOLS_DIR'), '\\', '/')
if MV_TOOLS_DIR is None:
  print 'ERROR: MV_TOOLS_DIR has not been set in the environment!'
  sys.exit(1)
MV_TOOLS_DIR += '/'
MV_COMMON_BASE = MV_TOOLS_DIR + '../mdk/common/'
COMPONENTS = MV_COMMON_BASE + 'components/'
DRIVERS = MV_COMMON_BASE + 'drivers/' + MV_SOC_PLATFORM + '/'
SWCOMMON = MV_COMMON_BASE + 'swCommon/'
SWCOMMON_PLATFORM = SWCOMMON + MV_SOC_PLATFORM + '/'
BOARD = COMPONENTS + 'Board/leon_code/'
LIBC = MV_COMMON_BASE + 'libc/leon/'
CIF_GENERIC = COMPONENTS + 'CifGeneric/leon_code/'

LEON_SOURCE += addSources(BOARD);
LEON_SOURCE += addSources(SWCOMMON_PLATFORM + 'src');
LEON_SOURCE += addSources(SWCOMMON + 'src');
LEON_SOURCE += addSources(DRIVERS + 'socDrivers');
LEON_SOURCE += addSources(CIF_GENERIC);
LEON_SOURCE += addSources(DRIVERS + 'system/asm');
LEON_SOURCE += addSources(LIBC + 'src');
LEON_SOURCE += addSources(LIBC + 'src/asm');

CCOPT = (
  '-I ../include '
  '-I ../coretech/common/include '
  '-I ../coretech/messaging/include '
  '-I ../coretech/vision/include '
  '-I' + DRIVERS + 'socDrivers/include '
  '-I' + DRIVERS + 'brdDrivers/include '
  '-I' + DRIVERS + 'icDrivers/include '
  '-I' + BOARD + ' '
  '-I' + MV_COMMON_BASE + 'shared/include '
  '-I' + MV_COMMON_BASE + 'swCommon/include '
  '-I' + MV_COMMON_BASE + 'swCommon/' + MV_SOC_PLATFORM + '/include '
  '-I' + MV_COMMON_BASE + 'libc/leon/include '
  '-I' + LIBC + 'include '
  '-I' + CIF_GENERIC + ' '
  '-Os -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 '
  '-DMOVI_TOOLS '
)

CXXOPT = (
  '-std=c++0x '
)

OUTPUT = 'build/'

# Determine the path to sparc-elf-gcc
UNAME = subprocess.check_output("uname", shell=True)
if 'Linux' in UNAME:
  DETECTED_PLATFORM = 'linux32'
  SPARC_DIR = 'sparc-elf-' + GCC_VERSION + '/'
else:
  DETECTED_PLATFORM = 'win32'
  SPARC_DIR = 'sparc-elf-' + GCC_VERSION + '-mingw/'

PLATFORM = MV_TOOLS_DIR + '/' + MV_TOOLS_VERSION + '/' + DETECTED_PLATFORM + '/'

GCC_DIR = PLATFORM + SPARC_DIR

CC = GCC_DIR + 'bin/sparc-elf-gcc '
CXX = GCC_DIR + 'bin/sparc-elf-g++ '
LD =  GCC_DIR + 'bin/sparc-elf-ld '
MVCONV = PLATFORM + 'bin/moviConvert'

LINKER_SCRIPT = 'ld/custom.ldscript' #'ld/' + MV_SOC_PLATFORM + '_default_memory_map.ldscript'
LDOPT = (
  '-O9 -t --gc-sections -M -warn-common -L ld -T ' + LINKER_SCRIPT + ' '
)


srcToObj = { }

"""Remove path separators in name"""
def split(name):
  name = str.replace(name, '\\', '/')
  sp = name.split('../')
  obj = ''
  for s in sp:
    obj += s
  sp = obj.split('//')
  obj = ''
  for s in sp:
    obj += s
  obj = string.replace(obj, ':', '')
  return obj

"""Iteratively create the folder structure for dir in case the intermediate
directories don't exist"""
def createDir(dir):
  d = ''
  for s in dir.split('/'):
    d += s + '/'
    try:
      os.mkdir(d)
    except:
      pass

"""Parse the dependency file for src and check file modification times"""
def areDependenciesCurrent(src, obj, obj_time):
  dname = obj + '.d'
  if not os.path.isfile(dname):
    return False
  
  str = ''
  with open(dname) as f:
    for line in f.readlines():
      str += line
  str = string.replace(str, '\\', ' ')
  str = string.replace(str, '\r', ' ')
  str = string.replace(str, '\n', ' ')
  for sp in str.split(' '):
    s = sp
    if s.endswith(':'):
      s = s[0:len(s)-1]
    if s != obj and s != src and s != src and s != '':
      stat = os.stat(s)
      if stat.st_mtime >= obj_time:
        return False
  
  return True

"""Invoke the LEON/sparc compiler for src and generate dependencies"""
def compileLEON(src):
  if not src.endswith('.S') and not src.endswith('.c') and not src.endswith('.cpp') and not src.endswith('.cc'):
    return
  
  obj = OUTPUT + split(src) + '.o'
  srcToObj[src] = obj
  dir = obj[0:obj.rfind('/')]
  createDir(dir)
  
  needsCompile = True
  if os.path.isfile(obj):
    srcStat = os.stat(src)
    objStat = os.stat(obj)
    if srcStat.st_mtime <= objStat.st_mtime and areDependenciesCurrent(src, obj, objStat.st_mtime):
      needsCompile = False
  
  if needsCompile:
    print 'Compiling:', src[(src.rfind('/') + 1):]
    if src.endswith('.S'):
      if os.system(CC + ' -c ' + CCOPT + ' -DASM ' + src + ' -o ' + obj) != 0:
        sys.exit(1)
      os.system(CC + ' ' + CCOPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src)
    elif src.endswith('.c'):
      if os.system(CC + ' -c ' + CCOPT + ' ' + src + ' -o ' + obj) != 0:
        sys.exit(1)
      os.system(CC + ' ' + CCOPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src)
    elif src.endswith('.cpp') or src.endswith('.cc'):
      if os.system(CXX + ' -c ' + CCOPT + ' ' + CXXOPT + ' ' + src + ' -o ' + obj) != 0:
        sys.exit(1)
      os.system(CXX + ' ' + CCOPT + ' ' + CXXOPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src)

if __name__ == '__main__':
  isTest = False

  for arg in sys.argv:
    if arg == 'clean':
      print 'Cleaning...'
      os.system('rm -rf ' + OUTPUT + '*')
      sys.exit(0)
    elif arg == 'vision-tests':
      isTest = True
      TARGET = 'vision-tests'
      LEON_SOURCE += addSources('../coretech/vision/robot/project/myriad1/unitTests/leon')
      LEON_SOURCE += addSources('../coretech/vision/robot/src')
      LEON_SOURCE += addSources('../coretech/vision/robot/test')
      SHAVE_SOURCE += addSources('../coretech/vision/robot/project/myriad1/unitTests/shave')
    elif arg == 'common-tests':
      isTest = True
      TARGET = 'common-tests'
      LEON_SOURCE += addSources('../coretech/common/robot/project/myriad1/unitTests/leon')
      LEON_SOURCE += addSources('../coretech/common/robot/test')
      LEON_SOURCE += addSources('../coretech/common/robot/src/')
      LEON_SOURCE += addSources('../coretech/common/shared/src/')
      SHAVE_SOURCE += addSources('../coretech/common/robot/project/myriad1/unitTests/shave')

  # TODO: Get supervisor directory building/linking
  if not isTest:
    LEON_SOURCE += addSources('./supervisor/src')
    pass
  
  for src in (LEON_SOURCE):
    compileLEON(src)
  
  objects = ''
  for key in srcToObj.keys():
    objects += ' ' + srcToObj[key]
  
  print 'Linking ' + TARGET + '.elf'
  file = OUTPUT + TARGET + '.elf'
  s = LD + ' ' + LDOPT + ' -o ' + file + ' ' + objects + ' > ' + OUTPUT + TARGET + '.map'
  if os.system(s) != 0:
    sys.exit(1)

  s = MVCONV + ' -elfInput ' + file + ' -mvcmd:' + OUTPUT + TARGET + '.mvcmd'
  if os.system(s) != 0:
    sys.exit(1)

  # Output the flash script
  with open(OUTPUT + TARGET + '.scr', 'w+') as f:
    size = os.stat(OUTPUT + TARGET + '.mvcmd').st_size
    f.write('breset\nstart a\ntarget l\nddrinit\nddrinit\n')
    f.write('loadandverify ' + MV_COMMON_BASE + '/utils/jtag_flasher/flasher.elf\n')
    f.write('set 0x48000000 ' + str(size) + '\n')
    f.write('load 0x48000004 bbe ' + TARGET + '.mvcmd\n')
    f.write('runw\n')


