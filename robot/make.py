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
USE_LE_COMPILER_TOOLS = False

MV_SOC_PLATFORM = 'myriad1'
if USE_LE_COMPILER_TOOLS:
  GCC_VERSION = '4.4.2-LE'
else:
  GCC_VERSION = '4.4.2-mingw'
  
MV_TOOLS_VERSION = '00.50.39.2'

LEON_SOURCE = []
LEON_SOURCE += addSources('./hal')
LEON_SOURCE += addSources('./hal/asm')
LEON_SOURCE += addSources('./vision')
LEON_SOURCE += addSources('../coretech/vision/robot/src')
LEON_SOURCE += addSources('../coretech/messaging/robot/src')
LEON_SOURCE += addSources('../coretech/common/robot/src')
LEON_SOURCE += addSources('../coretech/common/shared/src')

SHAVES_TO_USE = [0] # Can be any set of numbers from 0-7 for myriad1
SHAVE_SOURCE = []
SHAVE_SOURCE += addSources('../coretech/common/robot/src/shave')
SHAVE_SOURCE += addSources('../coretech/vision/robot/src/shave')

MV_TOOLS_DIR = os.environ.get('MV_TOOLS_DIR')

if os.environ.get('MV_TOOLS_DIR') is None:
  print('WARNING: MV_TOOLS_DIR has not been set in the environment!')
  print('In Linux, add the following to your ".bashrc" file: export MV_TOOLS_DIR={full path of movidius-tools/tools}')
  print('In Windows 7, add it to Control Panel->System->Advanced System Settings->Advanced->Environment Variables')
  print('Using default ../../movidius-tools/tools')
  MV_TOOLS_DIR = '../../movidius-tools/tools'

MV_TOOLS_DIR = str.replace(MV_TOOLS_DIR, '\\', '/')
MV_TOOLS_DIR += '/'
MV_COMMON_BASE = MV_TOOLS_DIR + '../mdk/common/'
COMPONENTS = MV_COMMON_BASE + 'components/'
DRIVERS = MV_COMMON_BASE + 'drivers/' + MV_SOC_PLATFORM + '/'
SWCOMMON = MV_COMMON_BASE + 'swCommon/'
SWCOMMON_PLATFORM = SWCOMMON + MV_SOC_PLATFORM + '/'
BOARD = COMPONENTS + 'Board/leon_code/'
LIBC = MV_COMMON_BASE + 'libc/leon/'
CIF_GENERIC = COMPONENTS + 'CifGeneric/leon_code/'
CORETECH_EXTERNAL_DIRECTORY = '../../coretech-external/'

MOVICOMPILE_LIBRARY_PATH = MV_TOOLS_DIR + MV_TOOLS_VERSION + '/common/moviCompile/libraries/' + MV_SOC_PLATFORM + '/'
MOVICOMPILE_LIBRARIES = [MOVICOMPILE_LIBRARY_PATH + file for file in ['mlibcxx.a', 'mlibVecUtils.a', 'mlibc.a', 'compiler-rt.a']]

LEON_SOURCE += addSources(BOARD);
LEON_SOURCE += addSources(SWCOMMON_PLATFORM + 'src');
LEON_SOURCE += addSources(SWCOMMON + 'src');
LEON_SOURCE += addSources(DRIVERS + 'socDrivers');
LEON_SOURCE += addSources(CIF_GENERIC);
LEON_SOURCE += addSources(LIBC + 'src');
LEON_SOURCE += addSources(LIBC + 'src/asm');
LEON_SOURCE.append(CORETECH_EXTERNAL_DIRECTORY + 'heatshrink/heatshrink_decoder.cpp')
LEON_SOURCE.append(CORETECH_EXTERNAL_DIRECTORY + 'heatshrink/heatshrink_encoder.cpp')

SHAVE_SOURCE += addSources(SWCOMMON + 'shave_code/' + MV_SOC_PLATFORM + '/myriad1/asm')
SHAVE_SOURCE += addSources(SWCOMMON + 'shave_code/' + MV_SOC_PLATFORM + '/myriad1/src')

LEON_ASM_C_CXX_OPT = (
  '-DDISABLE_LEON_DCACHE '
#  '-DDISABLE_LEON_CACHE '
  '-DROBOT_HARDWARE '
  '-DCOZMO_ROBOT ' # TODO: ROBOT_HARDWARE and COZMO_ROBOT should probably be merged
  '-DANKICORETECH_EMBEDDED_USE_HEATSHRINK '
  '-I ../include '
  '-I supervisor/src '
  '-I ../coretech/common/include '
  '-I ../coretech/messaging/include '
  '-I ../coretech/vision/include '
  '-I ' + CORETECH_EXTERNAL_DIRECTORY + 'heatshrink '
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
  '-O2 -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 '  
)

LEON_C_OR_ASM_ONLY_OPT = (' ')

if USE_LE_COMPILER_TOOLS:
  # TODO: Maybe remove some of these later
  LEON_ASM_C_CXX_OPT.append('-Wextra -fno-inline-small-functions -mbig-endian --sysroot=. -fno-inline-functions-called-once -DDRAM_SIZE_MB=64 ')
  LEON_C_OR_ASM_ONLY_OPT = (
    '-Werror-implicit-function-declaration '
  )

LEON_CXX_ONLY_OPT = (
  '-std=c++0x '
)

if USE_LE_COMPILER_TOOLS:
  SHAVE_INCLUDES = (
    '-I ../include '
    '-I ../coretech/common/include '
    '-I ../coretech/vision/include '
    '-I ' + MV_TOOLS_DIR + MV_TOOLS_VERSION + '/common/moviCompile/include '
    '-I ' + MV_COMMON_BASE + '/swCommon/shave_code/' + MV_SOC_PLATFORM + '/include '
    '-I ' + MV_COMMON_BASE + '/shared/include '
  )

  SHAVE_MOVICOMPILE_OPT = (
    '-target-cpu ' + MV_SOC_PLATFORM + ' '
    '-S -O2 -ffunction-sections -globalstack -fno-inline-functions '
    + SHAVE_INCLUDES +
    '-D' + MV_SOC_PLATFORM.upper() + ' '
    '-DROBOT_HARDWARE '
  )

  SHAVE_MOVIASM_OPT = (
    '-cv:' + MV_SOC_PLATFORM + ' '
    '-a '
    + SHAVE_INCLUDES.replace('-I','-i:').replace('-i: ', '-i:') +
    '-i:' + MV_COMMON_BASE + '/swCommon/shave_code/' + MV_SOC_PLATFORM + '/asm ' # TODO: Why is this line included?
    '-elf '
  )

  SHAVE_MVLIB_LD_OPT = (
    '-r -EB '
  )

OUTPUT = 'build/'

SPARC_DIR = 'sparc-elf-' + GCC_VERSION + '/'

# Determine the path to sparc-elf-gcc
try:
  UNAME = subprocess.check_output("uname", shell=True)
  if 'Linux' in UNAME:
    DETECTED_PLATFORM = 'linux32'    
  else:
    DETECTED_PLATFORM = 'win32'
except:
  print('WARNING: No uname installed, assuming win32')
  DETECTED_PLATFORM = 'win32'

PLATFORM = MV_TOOLS_DIR + MV_TOOLS_VERSION + '/' + DETECTED_PLATFORM + '/'

GCC_DIR = PLATFORM + SPARC_DIR
LEON_ASM_C_CXX_OPT += ' -I ' + GCC_DIR + 'lib/gcc/sparc-elf/4.4.2/include/ '
LEON_ASM_C_CXX_OPT += ' -I ' + GCC_DIR + 'lib/gcc/sparc-elf/4.4.2/include-fixed/ '

CC = GCC_DIR + 'bin/sparc-elf-gcc '
CXX = GCC_DIR + 'bin/sparc-elf-g++ '
LD = GCC_DIR + 'bin/sparc-elf-ld '
OBJCOPY = GCC_DIR + 'bin/sparc-elf-objcopy '

MVASM = PLATFORM + 'bin/moviAsm'
MVCOMPILE = PLATFORM + 'bin/moviCompile'
MVCONV = PLATFORM + 'bin/moviConvert'
#MVLINK = PLATFORM + 'bin/moviLink'

LINKER_SCRIPT = 'ld/custom.ldscript'
LDOPT = (
  '-O9 -t --gc-sections -M -warn-common -L ld -T ' + LINKER_SCRIPT + ' '
)

if USE_LE_COMPILER_TOOLS:
  LDOPT.append('-EB ')

leonSrcToObj = { }
shaveSrcToObj = { }

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
  leonSrcToObj[src] = obj
  dir = obj[0:obj.rfind('/')]
  createDir(dir)

  needsCompile = True
  if os.path.isfile(obj):
    srcStat = os.stat(src)
    objStat = os.stat(obj)
    if srcStat.st_mtime <= objStat.st_mtime and areDependenciesCurrent(src, obj, objStat.st_mtime):
      needsCompile = False

  if needsCompile:
    print 'Compiling Leon:', src[(src.rfind('/') + 1):]
    if src.endswith('.S'):
      stage1 = CC + ' -c ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_C_OR_ASM_ONLY_OPT + ' -DASM ' + src + ' -o ' + obj
      stage2 = CC + ' ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_C_OR_ASM_ONLY_OPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src
    elif src.endswith('.c'):
      stage1 = CC + ' -c ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_C_OR_ASM_ONLY_OPT + ' ' + src + ' -o ' + obj
      stage2 = CC + ' ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_C_OR_ASM_ONLY_OPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src
    elif src.endswith('.cpp') or src.endswith('.cc'):
      stage1 = CXX + ' -c ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_CXX_ONLY_OPT + ' ' + src + ' -o ' + obj
      stage2 = CXX + ' ' + LEON_ASM_C_CXX_OPT + ' ' + LEON_CXX_ONLY_OPT + ' -MF"' + obj + '.d" -MG -MM -MP -MT"' + obj + '" -MT"' + src + '" ' + src
    else:
      print 'ERROR: Weird file extension: ', src
      sys.exit(1)

    if isNoisy:
      print stage1
      print stage2
    if os.system(stage1) != 0:
      sys.exit(1)
    if os.system(stage2) != 0:
      sys.exit(1)

"""Invoke the SHAVE compiler for src and generate dependencies"""
def compileSHAVE(src):
  if not src.endswith('.asm') and not src.endswith('.c'):
    return

  #validExtensions = ['.asm', '.c']
  #if any((src.endswith(extension) for extension in validExtensions)) == True:
  #  return

  asmgen = OUTPUT + split(src) + '.asmgen'
  obj = OUTPUT + split(src) + '.o'
  shaveSrcToObj[src] = obj
  dir = obj[0:obj.rfind('/')]
  createDir(dir)

  needsCompile = True # Yins think this needs compile, nat?
  if os.path.isfile(obj):
    srcStat = os.stat(src)
    objStat = os.stat(obj)

    if srcStat.st_mtime <= objStat.st_mtime and areDependenciesCurrent(src, obj, objStat.st_mtime):
      needsCompile = False

  if needsCompile:
    print('Compiling Shave: ' + src[(src.rfind('/') + 1):])
    if src.endswith('.asm'):
      stage1 = MVASM + ' ' + SHAVE_MOVIASM_OPT + src + '-o:' + obj
    elif src.endswith('.c'):
      stage1 = MVCOMPILE + ' ' + SHAVE_MOVICOMPILE_OPT + ' ' + src + ' -o ' + asmgen
      stage2 = MVASM + ' ' + SHAVE_MOVIASM_OPT + ' ' + asmgen + ' -o:' + obj
    else:
      print('ERROR: Weird file extension: ' + src)
      sys.exit(1)

    if isNoisy:
      print(stage1)
      print(stage2)

    if os.system(stage1) != 0:
      sys.exit(1)

    if 'stage2' in locals():
      if os.system(stage2) != 0:
        sys.exit(1)

def linkSHAVEMvlib(mvlibFilename):
  print('Linking Shave Mvlib library')

  compiledObjectsString = ' '.join(shaveSrcToObj.values())
  moviCompileLibrariesString = ' '.join(MOVICOMPILE_LIBRARIES)

  # NOTE: The Myriad makefile creates an intermediate libray "swCommon.mvlib",
  #       but this command compiles our shave code and all the Myriad components together as .o files,
  #       and the closed-source moviCompile libraries as .a files
  systemString = LD + ' ' + SHAVE_MVLIB_LD_OPT + compiledObjectsString + ' ' + moviCompileLibrariesString + ' -o ' + mvlibFilename

  if isNoisy:
    print(systemString)

  if os.system(systemString) != 0:
    sys.exit(1)

def linkSHAVEShvlib(mvlibFilename, shvlibFilename, shaveNumber, prefixSymbolsString):
  print('Linking Shave Shvlib library')

  mustEndWith = '.shv' + str(shaveNumber) + 'lib'
  if not shvlibFilename.endswith(mustEndWith):
    print('Error: shvlibFilename must end with ' + mustEndWith)
    sys.exit(1)

  systemString = OBJCOPY + ' ' + '--prefix-alloc-sections=.shv' + str(shaveNumber) + '. --prefix-symbols=' + prefixSymbolsString + str(shaveNumber) + '_' + ' ' + mvlibFilename + ' ' + shvlibFilename

  if isNoisy:
    print(systemString)

  if os.system(systemString) != 0:
    sys.exit(1)

if __name__ == '__main__':
  isTest = False
  isRun = False
  isFlash = False
  isNoisy = False
  emulateShave = True
  quitDebuggerOnCompletion = False

  # Check if the Movidius tools can be found
  if any((os.path.isfile(file.strip()) or os.path.isfile(file.strip()+'.exe') for file in [CC, CXX, LD, MVCONV])) == False:
    print('Error: Could not locate all of the Movidius Tools')
    sys.exit(1)

  for arg in sys.argv:
    if arg == 'clean':
      print 'Cleaning...'
      if DETECTED_PLATFORM == 'win32':
        os.system('rmdir /s/q build')
      else:
        os.system('rm -rf ' + OUTPUT + '*')
      sys.exit(0)
    elif arg == 'vision-tests':
      isTest = True
      TARGET = 'vision-tests'
      LEON_SOURCE += addSources('../coretech/vision/robot/src')
      LEON_SOURCE += addSources('../coretech/vision/robot/test')
    elif arg == 'common-tests':
      isTest = True
      TARGET = 'common-tests'
      LEON_SOURCE += addSources('../coretech/common/robot/test')
      LEON_SOURCE += addSources('../coretech/common/robot/src/')
      LEON_SOURCE += addSources('../coretech/common/shared/src/')
    elif arg == 'run':
      isRun = True
    elif arg == 'flash':
      isFlash = True
    elif arg == 'make.py':
      pass
    elif arg == 'noisy':
      isNoisy = True
    elif arg == 'emulateShave':
      emulateShave = True
    elif arg == 'realShave':
      if USE_LE_COMPILER_TOOLS:
        print('Cannot use SHAVE with the non-little-endian compiler tools')
        sys.exit(-1);
        
      emulateShave = False
    elif arg == 'quit':
      quitDebuggerOnCompletion = True
    elif arg == 'capture-images':
      target = 'capture-images'
      LEON_ASM_C_CXX_OPT += '-DUSE_OFFBOARD_VISION=0 -DUSE_CAPTURE_IMAGES '
      LEON_ASM_C_CXX_OPT += '-DDEFAULT_BAUDRATE=500000 '
    else:
      print 'Invalid argument: ' + arg
      sys.exit(1)
      
  if DETECTED_PLATFORM == 'win32':
    os.environ['CYGWIN'] = 'nodosfilewarning'
      
  if isTest:
    LEON_ASM_C_CXX_OPT += '-DDEFAULT_BAUDRATE=1000000 '
  else:
    LEON_SOURCE += addSources('supervisor/src')
  
  if emulateShave:
    SHAVES_TO_USE = []
    LEON_ASM_C_CXX_OPT += '-DEMULATE_SHAVE_ON_LEON'
    LEON_SOURCE += addSources('../coretech/common/robot/src/shave')
    LEON_SOURCE += addSources('../coretech/vision/robot/src/shave')
    SHAVE_SOURCE = []
  
  for shaveNumber in SHAVES_TO_USE:
    LEON_ASM_C_CXX_OPT += '-DUSE_SHAVE_' + str(shaveNumber) + ' '
   
  for src in (LEON_SOURCE):
    compileLEON(src)

  if not emulateShave:
    for src in SHAVE_SOURCE:
      compileSHAVE(src)

    shaveMvlibFilename = OUTPUT + TARGET + '.mvlib'
    linkSHAVEMvlib(shaveMvlibFilename)

  objects = ''

  for key in leonSrcToObj.keys():
    objects += ' ' + leonSrcToObj[key]
  
  if not emulateShave:
    shaveShvlibFilenames = [OUTPUT + TARGET + '.shv' + str(number) + 'lib' for number in SHAVES_TO_USE]
    for (shaveNumber, shvlibFilename) in zip(SHAVES_TO_USE, shaveShvlibFilenames):
      
      linkSHAVEShvlib(shaveMvlibFilename, shvlibFilename, shaveNumber, 'shave')
      objects += ' ' + shvlibFilename

  with open('ld/objects.ldscript', 'w+') as f:
    f.write('INPUT(' + objects + ' )\n')

  print 'Linking final library ' + TARGET + '.elf'
  file = OUTPUT + TARGET + '.elf'
  s = LD + ' ' + LDOPT + ' -o ' + file + ' > ' + OUTPUT + TARGET + '.map'

  if isNoisy:
    print(s)

  if os.system(s) != 0:
    sys.exit(1)

  s = MVCONV + ' -elfInput ' + file + ' -mvcmd:' + OUTPUT + TARGET + '.mvcmd'

  if isNoisy:
    print(s)

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

  # Output the debug script
  with open(OUTPUT + TARGET + 'DEBUG.scr', 'w+') as f:
    f.write('breset\nl ' + OUTPUT + TARGET + '.elf\nrun\n')
    
    if quitDebuggerOnCompletion:
      f.write('quit')

  if isRun:
    os.system('moviDebug -b:' + OUTPUT + TARGET + 'DEBUG.scr')


