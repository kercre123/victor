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

DISABLE_LEON_ICACHE 		?=no
DISABLE_LEON_DCACHE 		?=no
				    
ifeq ($(MV_SOC_PLATFORM),myriad1)
   LEON_ENDIAN         	       	?=BEND
   MV_DEFAULT_START_PROC_ID     ?=L
else ifeq ($(MV_SOC_PLATFORM),myriad2)
   LEON_ENDIAN         	       	?=LEND
   CCOPT               		+= -DMYRIAD2
   MVCCOPT             		+= -DMYRIAD2 
   ifeq ($(LEON_STARTUP_MASK),LRT)
    MV_DEFAULT_START_PROC_ID    ?=LRT
   else
    MV_DEFAULT_START_PROC_ID    ?=LOS
   endif
else 
   $(error "Invalid platform, please set MV_SOC_PLATFORM to myriad1 or myriad2")
endif           

GCCVERSION          ?= 4.4.2-LE
GCCVERSION_NO_ENDIAN_SUFFIX ?= $(subst -LE,,$(GCCVERSION))

ifeq ($(DISABLE_LEON_ICACHE),yes)
	CCOPT               += -DDISABLE_LEON_ICACHE
endif
	
ifeq ($(DISABLE_LEON_DCACHE),yes)
	CCOPT               += -DDISABLE_LEON_DCACHE
endif

ifeq ($(MV_LEON_TRAP_TYPE),mvt)
  CCOPT += -DLEON_MVT 
endif
	   
ifeq ($(LEON_ENDIAN),LEND)
  ifeq ($(MV_SOC_OS), none)
    CCOPT                        += -mlittle-endian  -mlittle-endian-regorder
  endif
  LD_ENDIAN_OPT	        ?=-EL
else
  ifeq ($(MV_SOC_OS), none)
    CCOPT                        += -mbig-endian
  endif
  LD_ENDIAN_OPT	        ?=-EB
endif   
	  
####################################################################################
#                    Platform Specific Configuration                               #
####################################################################################
UNAME := $(shell uname)
FULL_PLATFORM := $(shell uname -a)

ifeq ($(UNAME),Linux)
#  ifeq ($(findstring x86_64,$(FULL_PLATFORM)),x86_64)
#    DETECTED_PLATFORM =linux64
#  else
    DETECTED_PLATFORM =linux32
#  endif
  ifeq ($(MV_SOC_OS), rtems)
    CCOPT += -DRTEMS
  endif
  SPARC_DIR?=sparc-elf-$(GCCVERSION)
else
  DETECTED_PLATFORM =win32
  ifeq ($(MV_SOC_OS), rtems)
    CCOPT += -DRTEMS
  endif
  SPARC_DIR?=sparc-elf-$(GCCVERSION)
  #allow user to set path as cygwin or DOS
  ifneq ($(MV_TOOLS_DIR),)
     #MV_TOOLS_DIR:=$(shell cygpath -m "$(MV_TOOLS_DIR)")
     MV_TOOLS_DIR:=$(shell cygpath "$(MV_TOOLS_DIR)")
  endif
endif

MV_GCC_TOOLS ?= sparc-elf

# User can provide their own input on where this stuff goes
MV_TOOLS_VERSION ?= 00.50.39.2

MV_TOOLS_DIR     ?= $(HOME)/WORK/Tools/Releases/General


# Variable for internal use only by this makefile
MV_TOOLS_BASE    = $(MV_TOOLS_DIR)/$(MV_TOOLS_VERSION)

#Name of the compiled rtems bsp directory
RTEM_BUILD_NAME		?=b-no-drvmgr

# RTEMS tools and bsp
MV_RTEMS_BASE ?= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/rtems/$(RTEM_BUILD_NAME)

MV_GCC_TOOLS_BASE ?= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)

MVLNK     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviLink
MVASM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviAsm
MVDUMP    		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDump
MVSIM     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviSim
MVDBG     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebug
MVDBGTCL  		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebugTcl
MVCONV    		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviConvert
MVSVR     		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviDebugServer

ifeq ($(findstring x86_64,$(FULL_PLATFORM)),x86_64)
MVCC      		= $(MV_TOOLS_BASE)/linux64/bin/moviCompile
else
MVCC      		= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/bin/moviCompile
endif

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
LEONOPTLEVEL ?= -Os

# Leon Tools common flags
ODOPT    		 += -xdsrw
SDOPT    		 += -t
LDOPT    		 += $(LD_ENDIAN_OPT) -O9 -t --gc-sections -M -warn-common -L $(DirSparcDefaultLibs) -L $(DirAppRoot) -L $(DirAppRoot)/scripts -L $(DirLDScript) -L $(DirLDScrCommon)

CCOPT    		 += --sysroot=. -fno-inline-functions-called-once
CCOPT                    += -Werror-implicit-function-declaration
CCOPT    		 += $(LEONOPTLEVEL) -mcpu=v8 -ffunction-sections -fno-common -fdata-sections -fno-builtin-isinff -gdwarf-2 -g3 $(WARN)

ifeq (,$(findstring 3.4.4,$(GCCVERSION)))
CCOPT    		 += -fno-inline-small-functions
endif

# Common usage option for VERBOSE=yes builds     
ifeq ($(VERBOSE),yes)
 ECHO = 
endif

#Options for dynamic loading linking
LDDYNOPT =-L . -L ./scripts -L ../../../common/scripts/ld --nmagic -s
LDDYNOPT+=-T $(MV_COMMON_BASE)/scripts/ld/$(MV_SOC_PLATFORM)_dynamic_shave_slice.ldscript

LDSYMOPT =-L . -L ./scripts -L ../../../common/scripts/ld --nmagic
LDSYMOPT+=-T $(MV_COMMON_BASE)/scripts/ld/$(MV_SOC_PLATFORM)_dynamic_shave_slice.ldscript
ASOPT    		 += $(CC_INCLUDE)

ifeq ($(DEBUG), yes)
 CCOPT += -DDEBUG
endif

ifeq ($(MV_SOC_OS), rtems)
  CCOPT += -isystem $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/$(MV_SOC_PLATFORM)/lib/include
  CCOPT += -isystem $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/sparc-elf/include
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
MVASMOPT         += -cv:$(MV_SOC_PLATFORM)  $(MVASMCODECHECKER)
#We'll change this once we have some better way of deaing with crt0init
#MVCCOPT          += -S $(MVCC_OPT_LEVEL) -ffunction-sections $(MVCCSTACK) -no-initialize-stack-in-main $(MVCOMPILEINLINESTATUS) $(MVCCDEBUG)
MVCCOPT          += -target-cpu $(MV_SOC_PLATFORM) -S $(MVCC_OPT_LEVEL) -ffunction-sections $(MVCCSTACK) $(MVCOMPILEINLINESTATUS) $(MVCCDEBUG)
#Need to define this later to keep parameters better aligned
MVLNKOPT         ?= -cv:$(MV_SOC_PLATFORM)
MVDBGOPT         ?= -cv:$(MV_SOC_PLATFORM)
MVSIMOPT         += -cv:$(MV_SOC_PLATFORM) -leon 
MVCMDOPT         ?= 
MVCMDOPT         += -ddrSize:$(DRAM_SIZE_MB)
MVSIMOUTOPT      ?= 
MVCONVOPT        ?= 
CCOPT            += -DDRAM_SIZE_MB=$(DRAM_SIZE_MB)

#It's ok to define MLNKOPT now
MVLNKOPT 	 += -I:$(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)
MVLNKOPT         += mlibc.mlib compiler-rt.mlib

MbinSymPrefix    ?= SVE0_

#################################################################################
#     Common libraries needed by tools to link all functionality                #
#################################################################################
#Leon common libraries
ifeq ($(MV_SOC_OS), rtems)
  DefaultSparcRTEMSLibs  += --start-group
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/$(MV_SOC_PLATFORM)/lib/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/lib/libbsp/sparc/$(MV_SOC_PLATFORM)/*.a)

  DefaultSparcRTEMSLibs  += $(wildcard $(MV_GCC_TOOLS_BASE)/$(MV_GCC_TOOLS)/lib/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_GCC_TOOLS_BASE)/lib/gcc/$(MV_GCC_TOOLS)/$(GCCVERSION_NO_ENDIAN_SUFFIX)/*.a)

  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/*/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/*/*/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/*/*/*/*.a)
  DefaultSparcRTEMSLibs  += $(wildcard $(MV_RTEMS_BASE)/$(MV_GCC_TOOLS)/c/$(MV_SOC_PLATFORM)/*/*/*/*/*.a)
  DefaultSparcRTEMSLibs  += --end-group
else
  DefaultSparcRTEMSLibs  ?=
endif
#Shave common libraries
CommonMlibFile   = $(DirAppOutput)/swCommon
ProjectShaveLib  = $(DirAppOutput)/$(APPNAME).mvlib
CompilerANSILibs  = $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/mlibcxx.a
ifeq ($(MV_SOC_PLATFORM),myriad1)
 # the VecUtils Library is currently only available for myriad1 see ticket #18939, this test should be removed when this ticket is resolved
 CompilerANSILibs += $(MV_TOOLS_BASE)/common/moviCompile/libraries/$(MV_SOC_PLATFORM)/mlibVecUtils.a
endif
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


ifndef LinkerScript
	ifdef  ENABLE_LINKER_PREPROCESS
		ifeq ($(filter $(notdir $(wildcard $(DirLDScript)/*.lds.S)), custom.lds.S), custom.lds.S)
		  LinkerScriptSource ?= $(DirLDScript)/custom.lds.S
		else
		  LinkerScriptSource ?= $(DirLDScrCommon)/$(MV_SOC_PLATFORM)/$(MV_SOC_PLATFORM)_default.lds.S
		endif
	    LinkerScript ?= $(DirAppOutput)/$(MV_SOC_PLATFORM)_default.lds
	else
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
endif

####################################################################################
#                    End of Mandatory common flags for some of the tools           #
####################################################################################
