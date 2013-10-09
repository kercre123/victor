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
   # Temporary default disable L1 for myriad2 testing only
   DISABLE_LEON_ICACHE ?=yes
   DISABLE_LEON_DCACHE ?=yes
   CCOPT               += -DMYRIAD2
   MVCCOPT             += -DMYRIAD2 
   # Note - this should be temporary:
   CCOPT               += -DBUILD_FOR_VCS
else 
   $(error "Invalid platform, please set MV_SOC_PLATFORM to myriad1 or myriad2")
endif           

GCCVERSION          ?= 4.4.2-LE

ifeq ($(DISABLE_LEON_ICACHE),yes)
	CCOPT               += -DDISABLE_LEON_ICACHE
endif
	
ifeq ($(DISABLE_LEON_DCACHE),yes)
	CCOPT               += -DDISABLE_LEON_DCACHE
endif

####################################################################################
#                    Platform Specific Configuration                               #
####################################################################################
UNAME := $(shell uname)
FULL_PLATFORM := $(shell uname -a)

ifeq ($(UNAME), Linux)
#  ifeq ($(findstring x86_64,$(FULL_PLATFORM)),x86_64)
#    DETECTED_PLATFORM =linux64
#  else
    DETECTED_PLATFORM =linux32
#  endif
  SPARC_DIR?=sparc-elf-$(GCCVERSION)
else
  DETECTED_PLATFORM =win32
  ifneq ($(LEON_ENDIAN),LEND)
    #SPARC_DIR?=sparc-elf-$(GCCVERSION)-mingw
    #We're using cygwin built tools nowadays, not postfixed with -mingw anymore:
    SPARC_DIR?=sparc-elf-$(GCCVERSION)
  endif  
  #allow user to set path as cygwin or DOS
  ifneq ($(MV_TOOLS_DIR),)
     #MV_TOOLS_DIR:=$(shell cygpath -m "$(MV_TOOLS_DIR)")
     MV_TOOLS_DIR:=$(shell cygpath "$(MV_TOOLS_DIR)")
  endif
endif

# User can provide their own input on where this stuff goes
MV_TOOLS_VERSION ?= 00.50.37.0

MV_TOOLS_DIR     ?= $(HOME)/WORK/Tools/Releases/General


# Variable for internal use only by this makefile
MV_TOOLS_BASE    = $(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)

MVLNK     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviLink
MVASM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviAsm
#MVASM     		= wine $(MV_TOOLS_BASE)/win32/bin/moviAsmLoHi.exe
MVDUMP    		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDump
MVSIM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviSim
MVDBG     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebug
MVDBGTCL  		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebugTcl
MVCONV    		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviConvert
#MVMOF     		= wine $(MV_TOOLS_BASE)/win32/bin/moviMof.exe
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
OBJDUMP         = $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/bin/sparc-elf-objdump

####################################################################################
#                    End of Platform Specific Configuration                        #
####################################################################################

####################################################################################
#                    Mandatory common flags for some of the tools                  #
####################################################################################

# Current Warning level 												  
#WARN			 ?= -Wall -Wextra
#Disabled by default for the moment since drivers are not ready for this
WARN			 ?= -Wextra

#Leon optimization level used
LEON_C_OPTIMIZATION_LEVEL ?= -Os
LEON_CPP_OPTIMIZATION_LEVEL ?= -O2

# Leon Tools common flags
CCOPT    		 += -fno-inline-functions-called-once
ifeq (,$(findstring 3.4.4,$(GCCVERSION)))
CCOPT    		 += -fno-inline-small-functions
endif
CCOPT            += -Werror-implicit-function-declaration
CCOPT    		 += $(LEON_C_OPTIMIZATION_LEVEL) -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 $(WARN)
CCOPT                    += -DDISABLE_LEON_ICACHE -DDISABLE_LEON_DCACHE -DDISABLE_LEON_CACHE

CPPOPT    		 += -fno-inline-functions-called-once
ifeq (,$(findstring 3.4.4,$(GCCVERSION)))
CPPOPT    		 += -fno-inline-small-functions
endif
CPPOPT    		 += $(LEON_CPP_OPTIMIZATION_LEVEL) -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 $(WARN)

ODOPT    		 += -xdsrw
SDOPT    		 += -t
LDOPT    		 += -O9 -t --gc-sections -M -warn-common -L $(DirSparcDefaultLibs) -L $(DirAppRoot) -L $(DirAppRoot)/scripts -L $(DirLDScript) -L $(DirLDScrCommon)

#Options for dynamic loading linking
LDDYNOPT =-L . -L ./scripts -L ../../../common/scripts/ld --nmagic -s
LDDYNOPT+=-T $(MV_COMMON_BASE)/scripts/ld/myriad1_dynamic_shave_slice.ldscript

LDSYMOPT =-L . -L ./scripts -L ../../../common/scripts/ld --nmagic
LDSYMOPT+=-T $(MV_COMMON_BASE)/scripts/ld/myriad1_dynamic_shave_slice.ldscript
ASOPT    		 += $(CC_INCLUDE)

ifeq ($(DEBUG), yes)
 CCOPT += -DDEBUG
endif

ifeq ($(DISABLE_PRINTF), yes)
 CCOPT += -DNO_PRINT
endif

CCOPT += $(CCOPT_EXTRA)
LDOPT += $(LDOPT_EXTRA)

LDOPT += -T $(LinkerScript)

EXTRA_MLIBS_PATH ?=
#These refer to moviCompile optimization levels
MVCC_OPT_LEVEL ?= -O2

#Extra C++ options to pass to moviCompile
MVCCPPOPT ?=

#These are arguments that will be passed to moviAsm for assembly files handwritten
#preferably like -opt:X. If nothing passed optimization level is considered 0. No need for -opt:0 because it is implicit
MVASM_HAND_OPT_LEVEL ?=
#These are arguments that will be passed to moviAsm for assembly files generated by moviCompile
#things like -optc 
MVASM_COMPILED_OPT_LEVEL ?=
#disable code checker
MVASMCODECHECKER ?= -a
#Movicompile stack options
MVCCSTACK ?= -globalstack
#moviCompile inline status
MVCOMPILEINLINESTATUS ?= -fno-inline-functions

#moviCompile debug options
ifeq ($(SHAVEDEBUG),yes)
MVCCDEBUG = -g
else
MVCCDEBUG =
endif

# Shave tools common flags
MVASMOPT         += $(MVASMCODECHECKER)
#We'll change this once we have some better way of deaing with crt0init
#MVCCOPT  		 += -S $(MVCC_OPT_LEVEL) -ffunction-sections $(MVCCSTACK) -no-initialize-stack-in-main $(MVCOMPILEINLINESTATUS) $(MVCCDEBUG)
MVCCOPT  		 += -S $(MVCC_OPT_LEVEL) -ffunction-sections $(MVCCSTACK) $(MVCOMPILEINLINESTATUS) $(MVCCDEBUG)
#Need to define this later to keep parameters better aligned
MVLNKOPT         ?=
MVCMDOPT         ?= 
MVCMDOPT         += -ddrSize:$(DRAM_SIZE_MB)
MVSIMOPT         += -leon
MVSIMOUTOPT      ?= 
MVCONVOPT        ?= 
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

#It's ok to define MLNKOPT now
MVLNKOPT 		 += -I:$(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)
MVLNKOPT         += mlibc.mlib compiler-rt.mlib

MbinSymPrefix    ?= SVE0_

#################################################################################
#     Common libraries needed by tools to link all functionality                #
#################################################################################
#CommonMlibFile   = $(DirAppOutput)/swCommon.mvlib
CommonMlibFile   = $(DirAppOutput)/swCommon
ProjectShaveLib  = $(DirAppOutput)/$(APPNAME).mvlib
CompilerANSILibs  = $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/mlibcxx.a
CompilerANSILibs += $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/mlibVecUtils.a
CompilerANSILibs += $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/mlibc.a
CompilerANSILibs += $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/compiler-rt.a

#Provide this to the users so they can use it in their applications if they include common shave code
PROJECT_SHAVE_LIBS = $(CommonMlibFile).mvlib $(CompilerANSILibs)

#Provide this to the users so they can use it in their applications if they include mbin shave code
PROJECT_SHAVE_MBINLIBS = $(CommonMlibFile).mlib

#if there are shave components then put the Project shave library on the list of dependencies
ifeq ($(SHAVE_COMPONENTS),yes)
PROJECT_SHAVE_LIBS += $(ProjectShaveLib)
endif

#This will need to hold shvXlib files to work 
#AllLibs  = $(DirAppOutput)/swCommon.shv0lib $(DirAppOutput)/swCommon.shv1lib
#AllLibs += $(DirAppOutput)/swCommon.shv2lib $(DirAppOutput)/swCommon.shv3lib
#AllLibs += $(DirAppOutput)/swCommon.shv4lib $(DirAppOutput)/swCommon.shv5lib
#AllLibs += $(DirAppOutput)/swCommon.shv6lib $(DirAppOutput)/swCommon.shv7lib

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
   LinkerScript	 ?= $(DirLDScrCommon)/$(MV_SOC_PLATFORM)_default_memory_map_elf.ldscript
 endif
endif
endif

####################################################################################
#                    End of Mandatory common flags for some of the tools           #
####################################################################################
