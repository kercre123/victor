
########################################################################
#                         General setup                                #
########################################################################

# Specify Default App name as being the same as the directory it is in
# Primary makefile can override
FULLPATH=$(shell pwd)
APPNAME ?= $(shell basename '$(FULLPATH)')

#Just make sure we have bin/bash as shell
ifneq ($(wildcard /bin/bash),)
    SHELL:=/bin/bash
    export SHELLOPTS:=errexit:pipefail
endif

# if MV_COMMON_BASE not set, then set
MV_COMMON_BASE      ?=../../common
MV_SOC_PLATFORM     ?= myriad1
MV_DRIVERS_BASE     ?=$(MV_COMMON_BASE)/drivers
MV_SWCOMMON_BASE    ?=$(MV_COMMON_BASE)/swCommon
MV_SWCOMMON_IC      ?=$(MV_COMMON_BASE)/swCommon/$(MV_SOC_PLATFORM)
MV_SHARED_BASE      ?=$(MV_COMMON_BASE)/shared
MV_EXTRA_DATA       ?=$(MV_COMMON_BASE)/../../mv_extra_data
MDK_BASE            ?=$(realpath $(MV_COMMON_BASE)/../)
DRAM_SIZE_MB        ?=64    

# Default test framework is Manual testing
# User makefiles can override this to be automatic
# Options: MANUAL, AUTO, CUSTOM
TEST_TYPE	    		?=MANUAL
TEST_TIMEOUT_SEC		?=300
TEST_HW_PLATFORM		?=MV0153_121
MDK_PROJ_STRING		    ?="MDK_TRUE"

DefaultAutoTestScriptName	 =autoTest.sh
DefaultAutoTestScript		 =$(MDK_BASE)/regression/jenkins/script/$(DefaultAutoTestScriptName)

# Default IP address for debugger connection, can be overridden		     
srvIP               ?= 127.0.0.1   
#srvPort             ?= 4001
srvPort             ?= 

# set verbosity to off by default
ECHO                ?= @

# Common usage option for VERBOSE=yes builds     
ifeq ($(VERBOSE),yes)
 ECHO                = 
endif
					    
# These parameters can be used to add additional Leon compiler or linker flags at make time		       
CCOPT_EXTRA         ?=
LDOPT_EXTRA         ?= 

#################################################################################
#                         Toolchain                                             #
#################################################################################
#To bring your project to GCC 4.4.2 from 3.4.4 you need to
#Remove any function prototypes that have unknown length arrays like:
#void myfunc(unsigned int vararray[][]);
#and make them valid like: void myfunc(unsigned int **vararray);
# and add the *(COMMON) section to your .bss sections in the ldscript of your application
#Like so:
#  .bss ALIGN(8) (NOLOAD) : { 
#    __bss_start = ALIGN(8);
#    *(.bss*8) *(.bss*4) *(.bss*2) *(.bss*1)
#    . = ALIGN(8); *(.bss)
#    . = ALIGN(8); *(.bss.*)
#    . = ALIGN(8); *(.bss*)
#    . = ALIGN(8); *(COMMON)
#    __bss_end = ALIGN(8);
#  }
#Or alternatively include myriad1_leon_default.ldscript in your script

ifeq ($(MV_SOC_PLATFORM),myriad1)
   # No definition implies myriad1 
   LEON_ENDIAN         ?=BEND
else ifeq ($(MV_SOC_PLATFORM),myriad2)
   # Latest tools revision required for little endian support for now
   MV_TOOLS_VERSION    ?=Latest
   LEON_ENDIAN         ?=LEND
   CCOPT_EXTRA         += -DMYRIAD2
else 
   $(error "Invalid platform, please set MV_SOC_PLATFORM to myriad1 or myriad2")
endif           


# Experimental Leon Little Endian mode
# NOT SUPPORTED.. Do NOT set => LEON_ENDIAN=LEND		      
ifeq ($(LEON_ENDIAN),LEND)
   GCCVERSION          ?= 4.4.2-LE
   CCOPT_EXTRA         += -DLENDIAN -Wa,-EL 
   LDOPT_EXTRA         += -EL
else  
   GCCVERSION          ?= 4.4.2
endif

ifeq ($(DISABLE_LEON_ICACHE),yes)
	CCOPT_EXTRA         += -DDISABLE_LEON_ICACHE
endif
	
ifeq ($(DISABLE_LEON_DCACHE),yes)
	CCOPT_EXTRA         += -DDISABLE_LEON_DCACHE
endif
		
####################################################################################
# Platform Specific Configuration
####################################################################################
UNAME := $(shell uname)
FULL_PLATFORM := $(shell uname -a)

ifeq ($(UNAME), Linux)
  DETECTED_PLATFORM =linux32
  SPARC_DIR?=sparc-elf-$(GCCVERSION)
else
  DETECTED_PLATFORM =win32
  ifneq ($(LEON_ENDIAN),LEND)
    SPARC_DIR?=sparc-elf-$(GCCVERSION)-mingw
  endif  
  #allow user to set path as cygwin or DOS
  ifneq ($(MV_TOOLS_DIR),)
     #MV_TOOLS_DIR:=$(shell cygpath -m "$(MV_TOOLS_DIR)")
     MV_TOOLS_DIR:=$(shell cygpath "$(MV_TOOLS_DIR)")
  endif
endif

# User can provide their own input on where this stuff goes
MV_TOOLS_VERSION ?= 00.50.35.1

MV_TOOLS_DIR     ?= $(HOME)/WORK/Tools/Releases/General


# Variable for internal use only by this makefile
MV_TOOLS_BASE    = $(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)

MVLNK     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviLink
MVASM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviAsm
MVDUMP    		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDump
MVSIM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviSim
MVDBG     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebug
MVDBGTCL  		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebugTcl
MVMOF     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviMof
ifeq ($(findstring x86_64,$(FULL_PLATFORM)),x86_64)
MVCC      		= $(MV_TOOLS_BASE)/linux64/bin/moviCompile
else
MVCC      		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviCompile
endif
MVSVR     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebugServer

# Sparc tools
CC              = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-gcc
LD              = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-ld
OBJCOPY         = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-objcopy
AS              = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-as
AR              = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-ar
STRIP           = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-strip

# This workaround is only needed for the big-endian tools release		      
ifeq ($(LEON_ENDIAN),BEND)
# This is a temporary work around for bugzilla #17256	   
ifeq ($(DETECTED_PLATFORM), linux32)
  OBJDUMP         = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/sparc-elf-3.4.4/bin/sparc-elf-objdump
else
  OBJDUMP         = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/sparc-elf-3.4.4-mingw/bin/sparc-elf-objdump
endif
else
  OBJDUMP       ?= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-objdump
endif


#####################################################################################
# Leon Tools default setup
# App root default to current directory

# default Directories
DirAppRoot           ?= .
DirLDScript          ?= $(DirAppRoot)/scripts
DirLDScrCommon       ?= $(MV_COMMON_BASE)/scripts/ld
DirDbgScrCommon      ?= $(MV_COMMON_BASE)/scripts/debug
DirAppOutput         ?= $(DirAppRoot)/output
DirAppObjDir         ?= $(DirAppOutput)/obj
DirAppMbinDir        ?= $(DirAppOutput)/mbin
DirAppSymDir         ?= $(DirAppOutput)/sym
DirOutputLst	     ?= $(DirAppOutput)/lst
DirAppScripts        ?= $(DirAppRoot)/scripts
DefaultFlashScript   ?= $(DirDbgScrCommon)/default_flash.scr
flashSCR             ?= $(DirAppOutput)/default_flash_script.scr
DefaultEraseScript   ?= $(DirDbgScrCommon)/default_erase.scr
eraseSCR             ?= $(DirAppOutput)/default_erase_script.scr
#Folder that holds default Sparc-elf-gcc libs
DirSparcDefaultLibs  ?= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/lib/gcc/sparc-elf/$(GCCVERSION)
#Any default libs specified. Need to be in the form of "-l[libraryname]"
DefaultSparcGccLibs  ?=

# Force a debug_run script to be created from default template unless one already exists
# By default we default to running an automatically generated debug script
# Applications which need to can override debugSCR in the top level makefile			   
ifeq ($(wildcard $(DirLDScript)/debug_run.scr),) 
   SourceDebugScript   ?= $(DirDbgScrCommon)/default_debug_run.scr
else
   SourceDebugScript   ?= $(DirLDScript)/debug_run.scr
endif
debugSCR             ?= $(DirAppOutput)/generated_debug_script.scr

LeonCodeSizeRpt      ?=$(DirOutputLst)/$(APPNAME)_LeonCodeSize.txt					    

# Leon generic setup, include path
CC_INCLUDE       = $(patsubst %,-I%,$(wildcard    $(DirLeonInclude) \
                                                  $(MV_DRIVERS_BASE)/brdDrivers/include \
                                                  $(MV_DRIVERS_BASE)/icDrivers/include \
                                                  $(MV_COMMON_BASE)/libc/leon/include \
                                                  $(MV_SWCOMMON_BASE)/include \
                                                  $(MV_SWCOMMON_IC)/include \
                                                  $(MV_SHARED_BASE)/include \
                                                  $(MV_SWCOMMON_BASE)/shave_code/$(MV_SOC_PLATFORM)/include \
                                                  $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/lib/gcc/sparc-elf/$(GCCVERSION)/include )) 

CC_INCLUDE      += $(patsubst %,-I%,$(wildcard    $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include \
                                                  $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/include \
                                                  $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/include))  

# Current Warning level 												  
#WARN			 ?= -Wall -Wextra
#Disabled by default for the moment since drivers are not ready for this
WARN			 ?= -Wextra

# Leon Tools common flags
CCOPT    		 += -fno-inline-functions-called-once
ifeq (,$(findstring 3.4.4,$(GCCVERSION)))
CCOPT    		 += -fno-inline-small-functions
endif
CCOPT            += -Werror-implicit-function-declaration
CCOPT    		 += -Os -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 $(WARN) $(CC_INCLUDE) 
CPPOPT                   += -O1 -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 $(WARN) $(CC_INCLUDE) 
ODOPT    		 += -xdsrw
SDOPT    		 += -t
LDOPT    		 += -O9 -t --gc-sections -M -warn-common -L $(DirSparcDefaultLibs) -L $(DirAppObjDir) -L $(DirLDScript) -L $(DirLDScrCommon) -T $(LinkerScript)
ASOPT    		 += $(CC_INCLUDE)

ifeq ($(DEBUG), yes)
 CCOPT += -DDEBUG
endif

ifeq ($(DISABLE_PRINTF), yes)
 CCOPT += -DNO_PRINT
endif

CCOPT += $(CCOPT_EXTRA)
LDOPT += $(LDOPT_EXTRA)

# Shave include
MVCC_INCLUDE      = $(patsubst %,-I %,$(wildcard   $(DirShaveInclude) \
                                                   $(MV_TOOLS_BASE)/common/moviCompile/include ))
MVCC_INCLUDE     += -I $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/include \
                    -I $(MV_SHARED_BASE)/include
					
EXTRA_MLIBS_PATH ?=
#These refer to moviCompile optimization levels
#MVCC_OPT_LEVEL ?= -O2
MVCC_OPT_LEVEL ?= -O0

#These are arguments that will be passed to moviAsm for assembly files handwritten
#preferably like -opt:X. If nothing passed optimization level is considered 0. No need for -opt:0 because it is implicit
MVASM_HAND_OPT_LEVEL ?=
#These are arguments that will be passed to moviAsm for assembly files generated by moviCompile
#things like -optc 
MVASM_COMPILED_OPT_LEVEL ?=
#disable code checker
MVASMCODECHECKER ?= 

# Shave tools common flags
MVASMOPT         += $(MVASMCODECHECKER) $(patsubst %,-i:%,$(wildcard $(DirShaveInclude))) -i:$(DirAppMbinDir) $(patsubst %,-i:%,$(wildcard $(ShaveCompDirs)))
#Adding common folder includes
MVASMOPT		 += -i:$(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/include -i:$(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/asm
MVCCOPT  		 += -S $(MVCC_OPT_LEVEL) $(MVCC_INCLUDE) -ffunction-sections
#Need to define this later to keep parameters better aligned
MVLNKOPT         ?=
MVCMDOPT         ?= 
MVCMDOPT         += -ddrSize:$(DRAM_SIZE_MB)
MVSIMOPT         += -leon
MVSIMOUTOPT      ?= 
MVMOFOPT         ?= 
CCOPT            += -DDRAM_SIZE_MB=$(DRAM_SIZE_MB)

ifeq ($(MV_SOC_PLATFORM),myriad2)
	MVSIMOPT         += -useXML:myriad2.xml
	MVICFLAG          = -cv:myriad2 
	MVASMOPT         += $(MVICFLAG)
	MVLNKOPT         += $(MVICFLAG) 
	MVLNKLEON         = -t:LOS
else	
#	MVICFLAG          = -cv:myriad1
	MVICFLAG          = 
	MVLNKLEON         = -t:L
endif

MbinSymPrefix    ?= SVE0_

# Default files
OutputFile		 ?= $(DirAppOutput)/$(APPNAME).mof
Updfile    		 ?= $(DirAppOutput)/$(APPNAME).upd
MvCmdfile    	 ?= $(DirAppOutput)/$(APPNAME).mvcmd
MlibFile		?= $(DirAppOutput)/$(APPNAME).mlib
CommonMlibFile   = $(DirAppOutput)/swCommon.mlib

#ifeq ($(filter $(notdir $(wildcard $(DirLDScript)/*.ldscript)), $(APPNAME).ldscript), $(APPNAME).ldscript)

#  LinkerScript	 ?= $(DirLDScript)/$(APPNAME).ldscript
#else
#  LinkerScript	 ?= $(DirLDScrCommon)/$(MV_SOC_PLATFORM)_default_memory_map.ldscript
#endif

 
ifndef LinkerScript
ifeq ($(filter $(notdir $(wildcard $(DirLDScript)/*.ldscript)), custom.ldscript), custom.ldscript)
  LinkerScript	 ?= $(DirLDScript)/custom.ldscript
else
ifeq ($(filter $(notdir $(wildcard $(DirLDScript)/*.ldscript)), $(APPNAME).ldscript), $(APPNAME).ldscript)
   LinkerScript	 ?= $(DirLDScript)/$(APPNAME).ldscript
  $(error You are using a deprecated way to give a customized .ldscript. Please, either \
     rename your ldscript file to "custom.ldscript", or set the "LinkerScript" variable in \
     your makefile! )
 else
   LinkerScript	 ?= $(DirLDScrCommon)/$(MV_SOC_PLATFORM)_default_memory_map.ldscript
 endif
endif
endif

##################################################################################
# MVT vs SVT for myriad 2
# could be svt = single vector trap  or 
#             mvt = multi vector trap

LeonTrapsType ?= svt


########################################################################################
# Leon and Shave source file locations
########################################################################################
# Add drivers/system files
SysCSrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/drvCommon/*/*.c) \
                     $(wildcard $(MV_SWCOMMON_BASE)/src/*.c)     \
                     $(wildcard $(MV_COMMON_BASE)/libc/leon/src/*.c)     \
                     $(wildcard $(MV_SWCOMMON_IC)/src/*.c)

SysASrcFiles      += $(wildcard $(MV_COMMON_BASE)/libc/leon/src/asm/*.S) 
ifeq ($(MV_SOC_PLATFORM),myriad2)
      SysASrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/traps/traps_$(LeonTrapsType).S) 
      ifeq ($(LeonTrapsType),mvt)
            CCOPT += -DLEON_MVT            
      endif 
endif
SysASrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/*.S) 


SysCSrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/*.c) 
SysCSrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/*.c) 
SysCSrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/*/*.c) 
SysCSrcFiles      += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/*.c) 
		   

# Default Leon C source files
LeonCSrcFiles     += $(wildcard $(DirAppRoot)/*leon*/*.c) \
                     $(wildcard $(DirAppRoot)/*/*leon*/*.c) \
                     $(wildcard $(DirAppRoot)/*/*/*leon*/*.c) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*.c) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*/*.c)

LeonCPPSrcFiles     += $(wildcard $(DirAppRoot)/*leon*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*/*leon*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*/*/*leon*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*/*.cpp)

# Default Leon ASM source files 
LeonASrcFiles     += $(wildcard $(DirAppRoot)/*leon*/*.S) \
                     $(wildcard $(DirAppRoot)/*/*leon*/*.S) \
                     $(wildcard $(DirAppRoot)/*/*/*leon*/*.S) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*.S) \
                     $(wildcard $(DirAppRoot)/*leon*/*/*/*.S)

# Default Header files
LeonHeaderFiles   += $(wildcard $(DirAppRoot)/*leon*/include/*.h) \
	                 $(wildcard $(DirAppRoot)/*/*leon*/include/*.h) \
	                 $(wildcard $(DirAppRoot)/*/*/*leon*/include/*.h) \
	                 $(wildcard $(DirAppRoot)/leon*/*/include/*.h) \
	                 $(wildcard $(DirAppRoot)/leon*/*/*.h) \
	                 $(wildcard $(DirAppRoot)/*.h) \
	                 $(wildcard $(DirAppRoot)/leon*/*/*/*.h) \
	                 $(wildcard $(DirAppRoot)/*/leon*/*.h) \
	                 $(wildcard $(DirAppRoot)/leon*/*.h)


# Shave ASm file list
ShaveASrcFiles    += $(wildcard $(DirAppRoot)/*shave*/*.asm) \
                     $(wildcard $(DirAppRoot)/*shave*/*/*.asm) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*/*.asm) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*.asm) \
                     $(wildcard $(DirAppRoot)/*/*/*shave*/*.asm)
                     
# Shave C file list
ShaveCSrcFiles    += $(wildcard $(DirAppRoot)/*shave*/*.c) \
                     $(wildcard $(DirAppRoot)/*shave*/*/*.c) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*/*.c) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*.c) \
                     $(wildcard $(DirAppRoot)/*/*/*shave*/*.c)

ShaveCPPSrcFiles  += $(wildcard $(DirAppRoot)/*shave*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*shave*/*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*.cpp) \
                     $(wildcard $(DirAppRoot)/*/*/*shave*/*.cpp)
					 
# Shave Header files
ShaveHeaderFiles  += $(wildcard $(DirAppRoot)/*shave*/*.h) \
                     $(wildcard $(DirAppRoot)/*shave*/*/*.h) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*.h) \
                     $(wildcard $(DirAppRoot)/*/*/*shave*/*.h) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*/*.h) \
                     $(wildcard $(DirAppRoot)/*shave*/*.hpp) \
                     $(wildcard $(DirAppRoot)/*shave*/*/*.hpp) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*/*.hpp) \
                     $(wildcard $(DirAppRoot)/*/*shave*/*.hpp) \
                     $(wildcard $(DirAppRoot)/*/*/*shave*/*.hpp)

# Application shared headers
ShaveHeaderFiles  += $(wildcard $(DirAppRoot)/*shared*/*.h)
ShaveHeaderFiles  += $(wildcard $(DirAppRoot)/*/*shared*/*.h)
ShaveHeaderFiles  += $(wildcard $(DirAppRoot)/*/*/*shared*/*.h)
LeonHeaderFiles   += $(wildcard $(DirAppRoot)/*shared*/*.h)
LeonHeaderFiles   += $(wildcard $(DirAppRoot)/*/*shared*/*.h)
LeonHeaderFiles   += $(wildcard $(DirAppRoot)/*/*/*shared*/*.h)


########################################################################
# Add components...
########################################################################

MoviLinkMlibPath =

# Pull components from Global common area
ifneq (ComponentList,)
  AllCompDirs         := $(patsubst %, $(MV_COMMON_BASE)/components/%,           $(ComponentList))
  SharedCompDirs      := $(patsubst %, $(MV_COMMON_BASE)/components/%/*shared*,  $(ComponentList))
  LeonCompDirs        := $(patsubst %, $(MV_COMMON_BASE)/components/%/*leon*,    $(ComponentList)) \
                         $(patsubst %, $(MV_COMMON_BASE)/components/%/*leon*/*,  $(ComponentList)) \
                      := $(patsubst %, $(MV_COMMON_BASE)/components/%/*leon*/*/*,$(ComponentList)) \
                      := $(patsubst %, $(MV_COMMON_BASE)/components/%/*leon*/*/*/*,  $(ComponentList))
  ShaveCompDirs       := $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*,   $(ComponentList)) \
                         $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*, $(ComponentList))

  SysCSrcFiles        += $(wildcard $(patsubst %, %/*.c,           $(LeonCompDirs) $(AllCompDirs) ))
  SysASrcFiles        += $(wildcard $(patsubst %, %/*.S,           $(LeonCompDirs) $(AllCompDirs) ))
  LeonHeaderFiles     += $(wildcard $(patsubst %, %/*.h,           $(LeonCompDirs) $(AllCompDirs) $(SharedCompDirs) ))
  LeonHeaderFiles     += $(wildcard $(patsubst %, %/include/*.h,   $(LeonCompDirs) $(AllCompDirs) $(SharedCompDirs) ))
  
  ShaveCSrcFiles      += $(wildcard $(patsubst %, %/*.c,           $(ShaveCompDirs)))
  ShaveCPPSrcFiles    += $(wildcard $(patsubst %, %/*.cpp,         $(ShaveCompDirs)))
  ShaveASrcFiles      += $(wildcard $(patsubst %, %/*.asm,         $(ShaveCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/*.h,           $(ShaveCompDirs) $(AllCompDirs) $(SharedCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/*.hpp,           $(ShaveCompDirs) $(AllCompDirs) $(SharedCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/include/*.h,   $(ShaveCompDirs) $(AllCompDirs) $(SharedCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/include/*.hpp,   $(ShaveCompDirs) $(AllCompDirs) $(SharedCompDirs)))
  
  LeonASrcFiles       += $(wildcard $(patsubst %, %/shavelib/*.S, $(AllCompDirs)))
  
  MoviLinkMlibIncludesPath    += $(patsubst %/, -I:%, $(sort $(dir $(shell echo $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*,   $(ComponentList)))))) \
                         $(patsubst %/, -I:%, $(sort $(dir $(shell echo $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*/*, $(ComponentList))))))
                         
  EXTRA_MLIBS ?= $(notdir $(wildcard $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*.mlib, $(ComponentList))))
endif

# Search for component from Local area
ifneq (LocalComponentList,)
  LocalAllCompDirs    := $(patsubst %, $(DirAppRoot)/%,         $(LocalComponentList))
  LocalLeonCompDirs   := $(patsubst %, $(DirAppRoot)/%/*leon*,  $(LocalComponentList)) \
                         $(patsubst %, $(DirAppRoot)/%/*leon*/*,  $(LocalComponentList))
  LocalShaveCompDirs  := $(patsubst %, $(DirAppRoot)/%/*shave*, $(LocalComponentList)) \
                         $(patsubst %, $(DirAppRoot)/%/*shave*/*, $(LocalComponentList))

  SysCSrcFiles        += $(wildcard $(patsubst %, %/*.c,           $(LocalLeonCompDirs) $(LocalAllCompDirs) ))
  SysASrcFiles        += $(wildcard $(patsubst %, %/*.S,           $(LocalLeonCompDirs) $(LocalAllCompDirs) ))
  LeonHeaderFiles     += $(wildcard $(patsubst %, %/*.h,           $(LocalLeonCompDirs) $(LocalAllCompDirs) ))
  LeonHeaderFiles     += $(wildcard $(patsubst %, %/include/*.h,   $(LocalLeonCompDirs) $(LocalAllCompDirs) ))
  
  ShaveCSrcFiles      += $(wildcard $(patsubst %, %/*.c,           $(LocalShaveCompDirs)))
  ShaveCPPSrcFiles    += $(wildcard $(patsubst %, %/*.cpp,         $(LocalShaveCompDirs)))
  ShaveASrcFiles      += $(wildcard $(patsubst %, %/*.asm,         $(LocalShaveCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/*.h,           $(LocalShaveCompDirs) $(LocalAllCompDirs)))
  ShaveHeaderFiles    += $(wildcard $(patsubst %, %/include/*.h,   $(LocalShaveCompDirs) $(LocalAllCompDirs)))
  
endif

#Even for the default case we still need some MLIBs in there because of the swCommon functionality
MDKMlibIncludePath = -I:$(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/include

SH_SWCOMMON_MLIB  = $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.c, %.mobj, $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.c))))
SH_SWCOMMON_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.cpp))))
SH_SWCOMMON_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.asm, %.mobj, $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.asm))))

#It's ok to define MLNKOPT now
MVLNKOPT 		 += -I:$(MV_TOOLS_BASE)/common/moviCompile/mlib -I:$(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM) $(EXTRA_MLIBS_PATH)

ifneq ($(SH_MLIB), )
MVLNKOPT         += $(MoviLinkMlibIncludesPath)
endif
MVLNKOPT         += $(MDKMlibIncludePath) -I:$(DirAppOutput)

# Currently we don't support these libraries on myriad2
ifneq ($(MV_SOC_PLATFORM),myriad2)
MVLNKOPT         += mlibc.mlib compiler-rt.mlib
endif

EXTRA_MLIBS		 += $(notdir $(CommonMlibFile))
ifneq ($(SH_MLIB), )
EXTRA_MLIBS      += $(notdir $(MlibFile))
endif
MVLNKOPT         += $(EXTRA_MLIBS)
MVLNKOPT         += -pad:16

GENERATED_ASM_OPT ?= -optc

# Add special extra Drivers objects
# Add extra objects. Developers may overwrite this
SpecificObjFiles ?=
#And extra cleaning
ExtraCleaningFiles ?=


##################################################################################
# Derive object files list from soruce files
##################################################################################
# Parse found include files and add to include list
DirLeonInclude   += $(patsubst %/,%,$(sort $(dir $(LeonHeaderFiles))))

# Derive system object files
SysObjFiles       = $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.c, %.o, $(SysCSrcFiles)))))
SysObjFiles      += $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.S, %.o, $(SysASrcFiles)))))

# Derive Leon Object files
LeonObjFiles      = $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.c, %.o, $(LeonCSrcFiles)))))
LeonObjFiles      += $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.cpp, %.o, $(LeonCPPSrcFiles)))))
LeonObjFiles     += $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.S, %.o, $(LeonASrcFiles)))))

# Derive Shave object files
ShaveMobjFiles    = $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.c,   %.mobj, $(ShaveCSrcFiles)))))
ShaveMobjFiles   += $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.cpp,   %.mobj, $(ShaveCPPSrcFiles)))))
ShaveMobjFiles   += $(addprefix $(DirAppObjDir)/, $(sort $(notdir $(patsubst %.asm, %.mobj, $(ShaveASrcFiles)))))

# Parse found include files and add to include list, and add Mbin dir
DirShaveInclude  += $(sort $(dir $(ShaveHeaderFiles)) $(DirAppMbinDir))



##################################################################################

# Shave common link name
ShaveCommonName  ?= shave_common

# Define which Mobj goes in which Shave
#SH0_Obj ?= $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.asm, %.mobj, $(ShaveASrcFiles))))
SH0_Obj ?=  $(DirAppObjDir)/$(ShaveCommonName).mobj
SH1_Obj ?= 
SH2_Obj ?=
SH3_Obj ?=
SH4_Obj ?=
SH5_Obj ?=
SH6_Obj ?=
SH7_Obj ?=
SHG_Obj ?=

SH_MLIB ?= 

# Only add to command line if object exists
SH_Obj  = $(if $(SH0_Obj),-t:S0 $(SH0_Obj),) \
          $(if $(SH1_Obj),-t:S1 $(SH1_Obj),) \
          $(if $(SH2_Obj),-t:S2 $(SH2_Obj),) \
          $(if $(SH3_Obj),-t:S3 $(SH3_Obj),) \
          $(if $(SH4_Obj),-t:S4 $(SH4_Obj),) \
          $(if $(SH5_Obj),-t:S5 $(SH5_Obj),) \
          $(if $(SH6_Obj),-t:S6 $(SH6_Obj),) \
          $(if $(SH7_Obj),-t:S7 $(SH7_Obj),) \
          $(if $(SHG_Obj),-t:G  $(SHG_Obj),)
 

# Default Shave consolidation Mof Name (ldscipt and .S are inferred)
ShaveSymObjFiles ?= $(DirAppObjDir)/$(ShaveCommonName).sym.o
ShaveCommonMof   ?= $(DirAppObjDir)/$(ShaveCommonName).mof


# Create VPATH from places where .c, .cpp, .h and .asm files where found
VPATH  +=$(sort $(dir $(LeonCSrcFiles) $(LeonCPPSrcFiles) $(LeonASrcFiles) $(LeonHeaderFiles) $(SysCSrcFiles)))
VPATH  +=$(sort $(dir $(ShaveCSrcFiles) $(ShaveCPPSrcFiles) $(ShaveASrcFiles) $(ShaveHeaderFiles)))
VPATH  +=$(MV_DRIVERS_BASE)/src/common:$(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm
VPATH  +=$(MV_DRIVERS_BASE)/src/common:$(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/traps
VPATH  +=$(MV_COMMON_BASE)/libc/leon/src/
VPATH  +=$(MV_COMMON_BASE)/libc/leon/src/asm
VPATH  +=$(MV_SWCOMMON_BASE)/shave_code/$(MV_SOC_PLATFORM)/asm
MOF    ?= $(DirAppOutput)/$(APPNAME).mof

# General make marker
.SECONDARY: 

.PRECIOUS: $(DirAppObjDir)/%.sym.S


##################################################################################
# Standard rules
##################################################################################

CHECKINSTALLOUTPUT=$(shell cd $(MV_COMMON_BASE)/utils && ./mincheck.sh)

all:
	@echo $(CHECKINSTALLOUTPUT)
ifeq ($(CHECKINSTALLOUTPUT),MV_TOOLS_DIR set)
	+make trueall
endif
	

#This is the true "all" target but we need to be able to both keep most of the process runnable with -j
#as that speeds things a lot and at the same time be able to check the installation everytime
trueall: $(OutputFile) $(MvCmdfile) $(MlibFile)

#all: $(OutputFile) $(MvCmdfile) $(MlibFile)

makeDebug:
	@echo SOC Platform:$(MV_SOC_PLATFORM)
	@echo Compiler Opts:
	@echo --------------------------
	@echo ${CCOPT}
	@echo Leon ASM Opts:
	@echo --------------------------
	@echo ${ASOPT}
	@echo Leon Linker Opts:
	@echo --------------------------
	@echo ${LDOPT}
	@echo Leon Source Files:
	@echo --------------------------
	@echo ${SysASrcFiles}
	@echo ${LeonCSrcFiles}
	@echo ${LeonCPPSrcFiles}
	
# Include the paths Makefile
-include $(MV_COMMON_BASE)/paths.mk

# final stage Mof flow
# General Purpose Linker rule (Mof variant)
$(DirAppOutput)/$(APPNAME).mof : $(DirAppOutput)/$(APPNAME)_leon.elf $(ShaveCommonMof)
	@echo "INFO: MOF Final Stage Link $@"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DirOutputLst)/$(APPNAME).map)
	$(ECHO) $(MVLNK)  $(MVICFLAG) -noLink $(MVLNKLEON) $(DirAppOutput)/$(APPNAME)_leon.elf -t:S0 $(ShaveCommonMof) -o:$(DirAppOutput)/$(APPNAME).mof -m:$(DirOutputLst)/$(APPNAME).map
	@mkdir -p $(dir $(DirOutputLst)/$(addsuffix .lst, $(notdir $(basename $@)))	)
	-$(ECHO) $(MVDUMP) $(MVICFLAG) $(DirAppOutput)/$(APPNAME).mof -o:$(DirOutputLst)/$(addsuffix .lst, $(notdir $(basename $@)))
	# [ Temp disable tools bug 17795] $(ECHO) $(MVMOF) $(MVICFLAG) $(MVMOFOPT) $(DirAppOutput)/$(APPNAME).mof -elf:$(DirAppOutput)/$(APPNAME).elf
	$(ECHO) $(MVMOF)             $(MVMOFOPT) $(DirAppOutput)/$(APPNAME).mof -elf:$(DirAppOutput)/$(APPNAME).elf
	# Temp disable error checking on objdump to debug LE problem
	-$(ECHO) $(OBJDUMP) $(SDOPT) $(DirAppOutput)/$(APPNAME).elf | egrep ^[0-9a-fA-F]{8} | egrep -v ^\(00000000\|1\[cd\]\) | sort > $(DirOutputLst)/$(APPNAME).elf.sym

# Project mlib
$(MlibFile) : $(SH_MLIB)
ifneq ($(SH_MLIB), )
	@echo "INFO: Mlib link: $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVLNK) $(MVICFLAG) -pad:16 $^ -o:$(MlibFile) -lib 
endif

# swCommon mlib
$(CommonMlibFile): $(SH_SWCOMMON_MLIB)
	@echo "INFO: Common Mlib link: $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVLNK) $(MVICFLAG) -pad:16 $^ -o:$(CommonMlibFile) -lib

# General Purpose Linker rule (elf variant)
$(DirAppOutput)/$(APPNAME)_leon.elf : $(LeonObjFiles) $(SpecificObjFiles) $(SysObjFiles) $(ShaveSymObjFiles) $(LinkerScript)
	@echo "INFO: ELF Leon Stage Link $(^)"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DirOutputLst)/$(APPNAME).map)
	$(ECHO) $(LD) $(LDOPT) -o $(@) $(SysObjFiles) $(SpecificObjFiles) $(LeonObjFiles) $(ShaveSymObjFiles) $(DefaultSparcGccLibs) > $(DirOutputLst)/$(addsuffix .map, $(notdir $(basename $@)))
	$(ECHO) $(OBJDUMP) $(ODOPT) $(@) > $(DirOutputLst)/$(addsuffix .lst, $(notdir $(basename $@)))	
	$(ECHO) $(OBJDUMP) $(SDOPT) $(@) | egrep ^[0-9a-fA-F]{8} | egrep -v ^\(00000000\|1\[cd\]\) | sort > $(DirOutputLst)/$(addsuffix .sym, $(notdir $(basename $@)))	


test2: $(LeonObjFiles)
	@echo "INFO: ELF Leon Stage Link $(^)"
	@echo $(LeonObjFiles)

test4: $(special)
	echo $(^)

# Calculate Leon Code Size
$(LeonCodeSizeRpt): $(DirAppOutput)/$(APPNAME)_leon.elf
	@echo "Generate summary of Leon function code sizes"
	-@grep .text $(DirOutputLst)/$(APPNAME)_leon.sym  | sed 's/\t/,/g' | sed 's/[ ]\+/,/g' | cut -d',' -f 5,6 | grep  ^0 > $(DirOutputLst)/$(APPNAME)csTmp1.csv
	-@cat  $(DirOutputLst)/$(APPNAME)csTmp1.csv  | xargs printf "0x%s\n" | sed 's/,/ /g' | xargs printf "%d %s\n" | sort -n > $(DirOutputLst)/$(APPNAME)csTmp2.csv  
	-@cat  $(DirOutputLst)/$(APPNAME)csTmp2.csv  | awk '{total = total + $$1}END{print "\nTotal Leon Code Size : " total " bytes"};' >> $(DirOutputLst)/$(APPNAME)csTmp2.csv    
#	-@sed -e  's/\n/\n\r/' $(DirOutputLst)/$(APPNAME)csTmp2.csv  >>  $(LeonCodeSizeRpt)  # tried to add dos line endings for notepad :-(
	-@cat $(DirOutputLst)/$(APPNAME)csTmp2.csv > $(LeonCodeSizeRpt)
	-@rm   $(DirOutputLst)/$(APPNAME)csTmp1.csv 
	-@rm   $(DirOutputLst)/$(APPNAME)csTmp2.csv 
	-@grep "Total Leon Code Size"   $(LeonCodeSizeRpt)  
	
gen_code_rpt: $(LeonCodeSizeRpt)

size_report: $(LeonCodeSizeRpt)
	cat $(LeonCodeSizeRpt)
	
# General Purpose for creating dependency trees
DepFiles = $(patsubst %.c,$(DirAppObjDir)/%.d,$(notdir $(LeonCSrcFiles))) 
DepFiles += $(patsubst %.cpp,$(DirAppObjDir)/%.d,$(notdir $(LeonCPPSrcFiles))) 
	
ifeq ($(wildcard $(DirAppObjDir)/*.d),)
  TakeHeaders	 = $(LeonHeaderFiles)
else
  TakeHeaders	 =
  include $(DepFiles)
endif

depend: $(DepFiles)
	@echo $(DepFiles)
 
	
$(DirAppObjDir)/%.d: %.c
	@echo "INFO: Creating Leon dependency file for $(<)"
	@mkdir -p $(dir $@)
	$(ECHO) $(CC) -MM $(CCOPT) $(filter %.c, $^) > $@
	$(ECHO) cp -f $@ $(DirAppObjDir)/$*.d.tmp
	$(ECHO) sed -e 's|..*:|$(patsubst %.d,%.o,$@):|' < $(DirAppObjDir)/$*.d.tmp > $@
	$(ECHO) rm -f $(DirAppObjDir)/$*.d.tmp	
	
# General Purpose Leon Compile rule
$(DirAppObjDir)/%.o: %.c $(TakeHeaders) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Compiling Leon c $(<)"
	@mkdir -p $(dir $@)
	@echo $(CC) -c $(CCOPT) $(filter %.c, $<) -o $@
	$(ECHO) $(CC) -c $(CCOPT) $(filter %.c, $<) -o $@

$(DirAppObjDir)/%.o: %.cpp $(TakeHeaders) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Compiling Leon c++ $(<)"
	@mkdir -p $(dir $@)
	@echo $(CC) -c $(CPPOPT) $(filter %.cpp, $<) -o $@
	$(ECHO) $(CC) -c $(CPPOPT) $(filter %.cpp, $<) -o $@

# General Purpose Leon Assembly
$(DirAppObjDir)/%.o: %.S $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Leon Assembling $(<)"
	@mkdir -p $(dir $@)
	$(ECHO) $(CC) $(CCOPT) -DASM -c -o $@ $<

# Shave Symbol file Assemble
$(DirAppObjDir)/%.sym.o: $(DirAppObjDir)/%.sym.S Makefile
	@echo "INFO: Assembling Shave symbol  $(<)"
	@mkdir -p $(dir $@)
	$(ECHO) $(AS) $(ASOPT) -o $@ $(DirAppObjDir)/$*.sym.S

# General Purpose Shave Assemble rule
$(DirAppObjDir)/%.mobj : %.asm  $(ShaveHeaderFiles) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Assembling $(@)"
	@mkdir -p $(dir $@)
	echo Building from : $<
	$(ECHO) $(MVASM) $(MVASMOPT) $(MVASM_HAND_OPT_LEVEL) $< -o:$@

ifeq ($(MV_SOC_PLATFORM),myriad2)
# Assemble Myriad2 assembly code into object file
$(DirAppObjDir)/%.mobj : $(DirAppObjDir)/%_gen_m2.asm $(ShaveHeaderFiles) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Movi Assembling Myriad2 generated asm $(@)"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVASM) $(MVASMOPT) $(MVASM_COMPILED_OPT_LEVEL) $(GENERATED_ASM_OPT) $< -o:$@
										  
# Convert M2 compiler generated Myriad1 assembly (_gen.asm) into Myriad2 compatible assembly (_gen_m2.asm)
$(DirAppObjDir)/%_gen_m2.asm : $(DirAppObjDir)/%_gen.asm $(ShaveHeaderFiles) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Movi Converting ASM M1->M2  $(@)"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVASM) -myriad2Replace $< -o:$@

else
# General Purpose Movi C compile rule [ Convertes Myriad1 compiler generated Assembly into object file]
$(DirAppObjDir)/%.mobj : $(DirAppObjDir)/%_gen.asm $(ShaveHeaderFiles) $(MV_TOOLS_BASE) Makefile
	@echo "INFO: Movi Assembling from generated asm $(@)"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVASM) $(MVASMOPT) $(MVASM_COMPILED_OPT_LEVEL) $(GENERATED_ASM_OPT) $< -o:$@
endif

$(DirAppObjDir)/%.i : %.c $(MV_TOOLS_BASE) Makefile
	@mkdir -p $(dir $@)
	@echo "INFO: Movi C preprocesing  $(@)"
	$(ECHO) $(MVCC) $(MVCCOPT) -E  $< -o $(DirAppObjDir)/$*.i

$(DirAppObjDir)/%_gen.asm : %.c $(MV_TOOLS_BASE) Makefile
	@mkdir -p $(dir $@)
	@echo "INFO: Movi C compiling  $(@)"
	$(ECHO) $(MVCC) $(MVCCOPT)  $< -o $(DirAppObjDir)/$*_gen.asm

$(DirAppObjDir)/%.ipp : %.cpp $(MV_TOOLS_BASE) Makefile
	@mkdir -p $(dir $@)
	@echo "INFO: Movi C++ preprocessing  $(^)"
	$(ECHO) $(MVCC) $(MVCCOPT)  -E $< -o $(DirAppObjDir)/$*.ipp
	
$(DirAppObjDir)/%_gen.asm : %.cpp $(MV_TOOLS_BASE) Makefile
	@mkdir -p $(dir $@)
	@echo "INFO: Movi C++ compiling  $(^)"
	@echo $(MVCC) $(MVCCOPT)  $< -o $(DirAppObjDir)/$*_gen.asm
	$(ECHO) $(MVCC) $(MVCCOPT)  $< -o $(DirAppObjDir)/$*_gen.asm

# General rule to make MVcmd from a Mof
$(DirAppOutput)/%.mvcmd : $(DirAppOutput)/%.mof
	@echo "INFO: Generating mvcmd target $(^)"
	@mkdir -p $(dir $@)
        # [temp disable tools bug 17795]   $(ECHO) $(MVMOF) $(MVICFLAG) $(MVCMDOPT) $(^) -mvcmd:$(@)
	$(ECHO) $(MVMOF)  $(MVCMDOPT) $(^) -mvcmd:$(@)

# General rule for mbin Symbol, inferring a pre-requisite
$(DirAppObjDir)/%.sym.S : $(DirAppMbinDir)/%.mbin
	@echo "";
    
# General rule for mbin, inferring a pre-requisite
# This line: $(subst $(subst ./,,$(CommonMlibFile)),,$(sort $(+)))
# removes $(CommonMlibFile) from build line as we don't need it here
$(DirAppMbinDir)/%.mbin : $(DirAppObjDir)/%.mobj $(CommonMlibFile)
	@echo "INFO: Generating mbin target $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVLNK) $(MVLNKOPT) -mbin $($*.Snum) $(subst $(subst ./,,$(CommonMlibFile)),,$(sort $(+))) -symPrefix:$(MbinSymPrefix) -o:$(DirAppMbinDir)/$*.mbin -ldscript:$(DirAppObjDir)/$*.ldscript -sym:$(DirAppObjDir)/$*.sym.S    
 
# General Purpose Shave Linker rule
$(DirAppObjDir)/%.mof : $(DirAppObjDir)/%.mobj $(MlibFile) $(CommonMlibFile)
	@echo "INFO: Shave Intermediate link : $@"
	@mkdir -p $(dir $@) 
	$(ECHO) $(MVLNK) $(MVLNKOPT) $(sort $(+)) -o:$(DirAppObjDir)/$*.mof -ldscript:$(DirAppObjDir)/$*.ldscript -sym:$(DirAppObjDir)/$*.sym.S    

# So getting make to produce two targets with one rule doesn't really work;  the rule gets called twice	
# Standard Shave Linker rule  (mof part)
$(DirAppObjDir)/$(ShaveCommonName).mof : $(SH0_Obj) $(SH1_Obj) $(SH2_Obj) $(SH3_Obj) $(SH4_Obj) $(SH5_Obj) $(SH6_Obj) $(SH7_Obj) $(MlibFile) $(CommonMlibFile)
	@echo "INFO: Shave Common link (for mof): $@"
	@mkdir -p $(dir $@)
	@echo $(MVLNK) $(MVLNKOPT) $(SH_Obj) -o:$(DirAppObjDir)/$(ShaveCommonName).mof 
	$(ECHO) $(MVLNK) $(MVLNKOPT) $(SH_Obj) -o:$(DirAppObjDir)/$(ShaveCommonName).mof 
	@mkdir -p $(dir $(DirOutputLst)/$(ShaveCommonName).lst)
	-$(ECHO) $(MVDUMP) $(MVICFLAG) $(DirAppObjDir)/$(ShaveCommonName).mof -o:$(DirOutputLst)/$(ShaveCommonName).lst  

# Standard Shave Linker rule (symbols part)
$(DirAppObjDir)/$(ShaveCommonName).sym.S : $(SH0_Obj) $(SH1_Obj) $(SH2_Obj) $(SH3_Obj) $(SH4_Obj) $(SH5_Obj) $(SH6_Obj) $(SH7_Obj) $(MlibFile) $(CommonMlibFile)
	@echo "INFO: Shave Common link (for symbols): $@"
	@mkdir -p $(dir $@)
	@mkdir -p $(dir $(DirOutputLst)/$(ShaveCommonName).map)
	$(ECHO) $(MVLNK) $(MVICFLAG) $(MVLNKOPT) $(SH_Obj) -o:$(DirAppObjDir)/dummy.mof -ldscript:$(DirAppObjDir)/shave_common.ldscript -sym:$(DirAppObjDir)/$(ShaveCommonName).sym.S -m:$(DirOutputLst)/$(ShaveCommonName).map    

# Common rules to simplify things	

run: $(MOF)
	@# The following line generates a custom debug script based on the current application name
	@# if the user has specified their own debug script then the sed part will replace nothing
	@# otherwise it will modify the template to use the correct mof name 
	@cat $(SourceDebugScript) | sed 's!XX_TARGET_MOF_XX!$(DirAppOutput)\/$(APPNAME).mof!' > $(debugSCR)
	$(MVDBG) -b:$(debugSCR) $(debugOptions) -srvIP:$(srvIP)

debug: $(MOF)
	@# The debug target differs from the run target in that it strips out any call to exit in the run script
	@# this allows the user to hit ctrl-c to abort runw and thus enter interactive debug mode
	@cat $(SourceDebugScript) | sed 's!XX_TARGET_MOF_XX!$(DirAppOutput)\/$(APPNAME).mof!' | grep -v exit > $(debugSCR) 
	$(MVDBG) -b:$(debugSCR) $(debugOptions) -srvIP:$(srvIP)

load: $(MOF)
	@# The load target differs from the debug target in that the last runw command is replaced with a run command.
	@# This allows the user to interactively type moviDebug commands while the application is running,
	@# without needing to stop the application.
	@cat $(SourceDebugScript) | sed 's!XX_TARGET_MOF_XX!$(DirAppOutput)\/$(APPNAME).mof!' | grep -v exit | sed '1!G;h;$$!d' | sed '0,/\brunw\b/s/\brunw\b/run/' | sed '1!G;h;$$!d' > $(debugSCR) 
	$(MVDBG) -b:$(debugSCR) $(debugOptions) -srvIP:$(srvIP)

debugi: 
	@# Simply launch moviDebug interactively
	$(MVDBG) $(debugOptions) -srvIP:$(srvIP)
				    
getTestType:
	@echo $(TEST_TYPE) $(TEST_HW_PLATFORM) Timeout:$(TEST_TIMEOUT_SEC)
	
# This target is used for unit testing   
autoTest:
	@echo "running Automated Test"
	mkdir -p output
	cp "$(DefaultAutoTestScript)" "$(DirAppOutput)/$(DefaultAutoTestScriptName)"
	export SELFTEST_SCRIPT_DIR="$(MDK_BASE)/regression/jenkins/script"; cd $(DirAppOutput); ./$(DefaultAutoTestScriptName) $(TEST_TYPE) $(TEST_TIMEOUT_SEC) "$(TARGET_PLATFORM)"
	
sim: $(MOF)
	$(MVSIM) $(MVSIMOPT) $(debugOptions) -l:$(DirAppOutput)/$(APPNAME).mof $(MVSIMOUTOPT) | tee $(DirOutputLst)/leonos_movisim.lst
	-$(ECHO)cat $(DirOutputLst)/leonos_movisim.lst  | grep "PC=0x" | sed 's/^PC=0x//g' > $(DirOutputLst)/leonos_movisim.dasm
	-$(ECHO)cat $(DirOutputLst)/leonos_movisim.lst  | grep -w "UART:\|DEBUG:" > $(DirOutputLst)/leonos_movisim_uart.txt

dump: $(MOF)
	-$(MVDUMP) $(MVICFLAG) $<

srv: start_server

start_server:
	while true; do $(MVSVR); done

start_simulator:
	while true; do $(MVSIM) -q; done

start_simulator_full:
	while true; do $(MVSIM) -darw; done
	
clean:
	rm -rf $(DirAppObjDir)/*.*
	rm -rf $(DirOutputLst)/*
	rm -rf $(DirAppOutput)/*.*
	rm -rf $(DirAppMbinDir)/*
	rm -f *.mof *.elf *.mobj *.obj *.mlib *.mbin *.sym.S *.o $(ExtraCleaningFiles)

#tentatively include targets used only for inhouse purposes: help with cleanup, ask projects to
#deliver certain information about themselves for the purposes of regressions etc.
-include $(MV_COMMON_BASE)/inhousetargets.mk

	
####################################################################################
# TCL configuration
####################################################################################
ifeq ($(UNAME), Linux)
  MOVI_TCL_INIT  		:= $(MDK_BASE)/regression/tcl/moviDebugStart.tcl
  MOVI_TCL_LIB   		:= $(realpath $(MV_TOOLS_BASE))/$(DETECTED_PLATFORM)/lib/moviDebugTcl.so
  MOVI_TCL_STARTUP_CMD 	:= rlwrap -I -c tclsh  ${MOVI_TCL_INIT}	 
#  MOVI_TCL_STARTUP_CMD 	:= tclsh  ${MOVI_TCL_INIT}	 
else
  # Temp hardcoded for testing TCL on cygwin
  # For now cygwin depends on windows Active TCL install rather than cygwin TCL
  # as cygwin TCL doesn't seem to support TclX
  # This also seems to prevent the successful use for rlwrap which makes the cygwin version pretty
  # useless. A further problem is the pressing of ctrl-c kills the app
  # Short Version: TCL for cygwin not yet working
  MOVI_TCL_INIT  := y://mdk/regression/tcl/moviDebugStart.tcl 
  MOVI_TCL_LIB   := y://Tools/Releases/Internal/Latest/$(DETECTED_PLATFORM)/lib/moviDebugTcl.dll
  #MOVI_TCL_LIB_DIR   := y: //Tools/Releases/Internal/Latest/$(DETECTED_PLATFORM)/lib/
  MOVI_TCL_STARTUP_CMD 	:= tclsh  ${MOVI_TCL_INIT}     	 
endif
MOVI_TCL_ENVIRONMENT  := export srvIP=$(srvIP);export srvPort=$(srvPort);export MOVI_TCL_LIB=${MOVI_TCL_LIB}; export MOVI_TCL_INIT=${MOVI_TCL_INIT}; export APP=$(MOF); export CMD=$(CMD);export MVCOM=$(realpath $(MV_COMMON_BASE))
					
tcl: 
	${MOVI_TCL_ENVIRONMENT} ; ${MOVI_TCL_STARTUP_CMD}
	
tclrun: $(MOF)
	make -C . tcl CMD=go

# Programs the target application into SPI Memory on MV0153
flash: MVCMDSIZE = $(shell du -b $(MvCmdfile))
flash: $(MvCmdfile)	
	$(ECHO)cat $(DefaultFlashScript) | \
		sed 's!XX_FLASHER_ELF_XX!$(MV_COMMON_BASE)/utils/jtag_flasher/flasher.elf!' | \
		sed 's!XX_TARGET_MVCMD_SIZE_XX!$(word 1, $(MVCMDSIZE))!' | \
		sed 's!XX_TARGET_MVCMD_XX!$(MvCmdfile)!' > $(flashSCR)
	$(ECHO)$(MVDBG) -b:$(flashSCR) $(debugOptions) -srvIP:$(srvIP)
	$(ECHO) if diff $(MvCmdfile) $(MvCmdfile).readback; then \
		echo "Reading back from DDR unchanged [OK]"; \
		rm $(MvCmdfile).readback; \
	else \
		echo "Reading back from DDR was different! [BAD]"; \
		exit 1; \
	fi
	
flash_erase:
	$(ECHO)cat $(DefaultEraseScript) | \
		sed 's!XX_FLASHER_ELF_XX!$(MV_COMMON_BASE)/utils/jtag_flasher/flasher.elf!' | \
		sed 's!XX_TARGET_MVCMD_SIZE_XX!0!' > $(eraseSCR)
	$(ECHO)$(MVDBG) -b:$(eraseSCR) $(debugOptions) -srvIP:$(srvIP)
	
check_install:
	@cd $(MV_COMMON_BASE)/utils && ./checkinstall.sh
	
-include $(MV_COMMON_BASE)/bigdata.mk

# This target is used by the build regression to detect which projects are MDK top-level projects
isMdkProject:
	@echo $(MDK_PROJ_STRING)
	
help:
	@echo "'make help'                      : show this message"
	@echo "'make all'                       : build everything"
	@echo "'make depend'                    : generate dependency trees for header files"
	@echo "'make size_report'               : Generate leon function code sizes report"
	@echo "'make run'                       : build everything and run it on a target via moviDebug"
	@echo "'make debug'                     : build everything and interactively debug it on a target via moviDebug. Need to press Ctrl-C to get a moviDebug prompt."
	@echo "'make load'                      : build everything and interactively debug it on a target via moviDebug. moviDebug prompt available while the application is running."
	@echo "'make debugi'                    : launch the debugger but don't load any application. For interactive session"
	@echo "'make sim'                       : build everything and run it in the simulator"
	@echo "'make start_server'              : starts moviDebug server"
	@echo "'make start_simulator'           : starts moviSim in quiet mode"
	@echo "'make start_simulator_full       : starts moviSim in verbose full mode"
	@echo "'make clean'                     : Clean build files"
	@echo "'make flash'                     : Program SPI flash of MV0153 with your current mvcmd"
	@echo "'make flash_erase'               : Erase SPI flash of MV0153"
	@echo "'make check_install'             : Check current MDK installation. WARNING: may not run. Run common/utils/checkinstall.sh"
	@echo 
	@echo "Optional Parameters:"
	@echo "srvIP=192.168.1.100              : Connect to moviDebugserver on a different PC"
	@echo "VERBOSE=yes                      : Echo full build commands during make"
	@echo "DISABLE_PRINTF=yes               : Turns off printf, useful when generating Boot image"
	@echo "DEBUG=yes                        : Enables -DDEBUG in compiler (causes asserts to print messages)"
	@echo "MvCmdfile=path/to/file.mvcmd     : given to \"make flash\" to write a different .mvcmd file"
	@echo 

