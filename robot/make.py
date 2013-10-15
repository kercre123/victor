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
LEON_SOURCE += addSources('./vision')
#LEON_SOURCE += addSources('./supervisor/src')
LEON_SOURCE += addSources('../coretech/vision/robot/src')
LEON_SOURCE += addSources('../coretech/messaging/robot/src')
LEON_SOURCE += addSources('../coretech/common/robot/src')

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
CIF_GENERIC = COMPONENTS + 'CifGeneric/leon_code/'
CIF_OV = COMPONENTS + 'CifOV5642/leon_code/'

DRIVER_SOURCE = [
DRIVERS + 'brdDrivers/brdMv0153/brdMv0153.c',

BOARD + 'Board.c',

SWCOMMON_PLATFORM + 'src/swcL2Cache.c',
SWCOMMON_PLATFORM + 'src/swcLedControl.c',
SWCOMMON_PLATFORM + 'src/swcMemoryTransfer.c',
SWCOMMON_PLATFORM + 'src/swcShaveLoader.c',
SWCOMMON_PLATFORM + 'src/swcSliceUtils.c',
SWCOMMON_PLATFORM + 'src/swcTestUtils.c',

SWCOMMON + 'src/swcCrc.c',
SWCOMMON + 'src/swcLeonMath.c',
SWCOMMON + 'src/swcLeonUtils.c',
SWCOMMON + 'src/swcRandom.c',

DRIVERS + 'icDrivers/icEeprom.c',
DRIVERS + 'icDrivers/icMipiTC358746.c',
DRIVERS + 'icDrivers/icPllCDCE913.c',
DRIVERS + 'icDrivers/icRegulatorLT3906.c',

DRIVERS + 'socDrivers/DrvCif.c',
DRIVERS + 'socDrivers/DrvCpr.c',
DRIVERS + 'socDrivers/DrvDdr.c',
DRIVERS + 'socDrivers/DrvGpio.c',
DRIVERS + 'socDrivers/DrvHsdio.c',
DRIVERS + 'socDrivers/DrvI2c.c',
DRIVERS + 'socDrivers/DrvI2cMaster.c',
DRIVERS + 'socDrivers/DrvI2s.c',
DRIVERS + 'socDrivers/DrvIcb.c',
DRIVERS + 'socDrivers/DrvL2Cache.c',
DRIVERS + 'socDrivers/DrvLcd.c',
DRIVERS + 'socDrivers/DrvPwm.c',
DRIVERS + 'socDrivers/DrvRegUtils.c',
DRIVERS + 'socDrivers/DrvSpi.c',
DRIVERS + 'socDrivers/DrvSvu.c',
DRIVERS + 'socDrivers/DrvSvuDebug.c',
DRIVERS + 'socDrivers/DrvTimer.c',
DRIVERS + 'socDrivers/DrvUart.c',

CIF_GENERIC + 'CIFGeneric.c',
CIF_OV + 'CifOV5642.c',

DRIVERS + 'system/asm/crt0.S',
DRIVERS + 'system/asm/memmap.S',
DRIVERS + 'system/asm/mp_rom.S',
DRIVERS + 'system/asm/sysbss.S',
DRIVERS + 'system/asm/traps.S'
]

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
	'-I' + CIF_GENERIC + ' '
	'-I' + CIF_OV + ' '
	'-Os -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 '
	'-DMOVI_TOOLS'
)

CXXOPT = (
	'-std=c++0x'
)

LINKER_SCRIPT = 'scripts/ld/' + MV_SOC_PLATFORM + '_default_memory_map.ldscript'
LDOPT = (
	'-O9 -t --gc-sections -M -warn-common -L scripts/ld -T ' + LINKER_SCRIPT
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

CC = PLATFORM + SPARC_DIR + 'bin/sparc-elf-gcc '
CXX = PLATFORM + SPARC_DIR + 'bin/sparc-elf-g++ '
LD = PLATFORM + SPARC_DIR + 'bin/sparc-elf-ld '

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
		if src.endswith('.S') or src.endswith('.c') or src.endswith('.cpp') or src.endswith('.cc'):
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
	for arg in sys.argv:
		if arg == 'clean':
			print 'Cleaning...'
			os.system('rm -rf ' + OUTPUT + '*')
			sys.exit(0)
		
	for src in (LEON_SOURCE + DRIVER_SOURCE):
		compileLEON(src)

	objects = ''
	for key in srcToObj.keys():
		objects += ' ' + srcToObj[key]

	print 'Linking ' + TARGET + '.elf'
	s = LD + ' ' + LDOPT + ' -o ' + OUTPUT + TARGET + '.elf ' + objects + ' > ' + OUTPUT + TARGET + '.map'
	os.system(s)

