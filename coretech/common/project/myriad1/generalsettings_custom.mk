##########################################################################
#                          GENERAL SETTINGS                              #
##########################################################################

#Some safe default setting: errors everywhere, and a nice app name

#verbose or not
ECHO ?= @

# Specify Default App name as being the same as the directory it is in
# Primary makefile can override
FULLPATH=$(shell pwd)
APPNAME ?= $(shell basename '$(FULLPATH)')

#Just make sure we have bin/bash as shell
ifneq ($(wildcard /bin/bash),)
    SHELL:=/bin/bash
    export SHELLOPTS:=errexit:pipefail
endif

MV_COMMON_BASE      ?=../../common
MV_SOC_PLATFORM     ?= myriad1
MV_DRIVERS_BASE     ?=$(MV_COMMON_BASE)/drivers
MV_SWCOMMON_BASE    ?=$(MV_COMMON_BASE)/swCommon
MV_SWCOMMON_IC      ?=$(MV_COMMON_BASE)/swCommon/$(MV_SOC_PLATFORM)
MV_SHARED_BASE      ?=$(MV_COMMON_BASE)/shared
MV_EXTRA_DATA       ?=$(MV_COMMON_BASE)/../../mv_extra_data
MV_LEON_LIBC_BASE   ?=$(MV_COMMON_BASE)/libc/leon
MDK_BASE            ?=$(realpath $(MV_COMMON_BASE)/../)
DRAM_SIZE_MB        ?=64
SHAVE_COMPONENTS    ?=no

# Default test framework is Manual testing
# User makefiles can override this to be automatic
# Options: MANUAL, AUTO, CUSTOM
TEST_TYPE	    		?=MANUAL
TEST_TIMEOUT_SEC		?=300
TEST_HW_PLATFORM		?=MV0153_121
MDK_PROJ_STRING		    ?="MDK_TRUE"

# default Directories
DirAppRoot           ?= .
DirLDScript          ?= $(DirAppRoot)/config
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

# Default files
OutputFile		 ?= $(DirAppOutput)/$(APPNAME).elf
MvCmdfile    	 ?= $(DirAppOutput)/$(APPNAME).mvcmd

#Folder that holds default Sparc-elf-gcc libs
DirSparcDefaultLibs  ?= $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/lib/gcc/sparc-elf/$(GCCVERSION)
#Any default libs specified. Need to be in the form of "-l[libraryname]"
DefaultSparcGccLibs  ?=

#Add object list of raw data object files.
#The main use of this is to include mbin files and testing data using sparc-elf-objcopy,
#but this may also be used if tables need to be loaded or LUTs and such, again, using sparc-elf-objcopy 
RAWDATAOBJECTFILES ?=

#defining global variables needed for clean and debugging of gnerated output
PROJECTINTERM ?=
PROJECTCLEAN ?=
    
# Force a debug_run script to be created from default template unless one already exists
# By default we default to running an automatically generated debug script
# Applications which need to can override debugSCR in the top level makefile			   
ifeq ($(wildcard $(DirLDScript)/debug_run.scr),) 
   SourceDebugScript   ?= $(DirDbgScrCommon)/default_debug_run_$(MV_SOC_PLATFORM).scr
else
   SourceDebugScript   ?= $(DirLDScript)/debug_run.scr
endif
debugSCR             ?= $(DirAppOutput)/generated_debug_script.scr
    
#Variable used to control checking of MV_TOOLS_DIR being set for external users
#This is so that internal Movidius users don't need to go through checking if
#MV_TOOLS_DIR is defined on their systems. This is related to feature request in
#Bug 18741 on the internal Movidius bugzilla
#!!!! Please DON'T MAKE ANY CHANGE to the follwoing line, NOT even adding small spaces or anything
#unless you also change the companion line in mdk/regression/mdk_release/Makefile -> 'pack' target
CHECK_TOOLS_VAR=yes

##########################################################################
#                       END OF GENERAL SETTINGS                          #
##########################################################################
