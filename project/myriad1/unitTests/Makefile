# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))
APPNAME=coretech-vision-unit-tests

MDK_DIRECTORY?=../../../../movidius-tools/mdk_13.08.0_general/mdk

CORETECH_COMMON_DIRECTORY?=../../../../coretech-common

MVCC_OPT_LEVEL ?= -O1
LEON_CPP_OPTIMIZATION_LEVEL ?= -O3

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk
#ShaveSymObject   = $(DirAppObjDir)/$(APPNAME).sym.o
#LinkerScript    =  $(DirLDScript)/$(APPNAME).ldscript

# Set Shave common mof name (non-mbin flow)
#ShaveCommonName = shaves

SH0_Obj += output/obj/shaveMain.mobj

ShaveCSrcFiles += shave/shaveMain.c 

LeonCSrcFiles += leon/main.c
LeonCPPSrcFiles += leon/cppInterface.cpp

LeonHeaderFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/include/)

LeonCPPSrcFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/src/embedded/*.cpp)

LeonCPPSrcFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/test/embedded/commonTests.cpp)

# Limit to only needed drivers
#SysCSrcFiles     = drv_ddr.c drv_uart.c drv_vcs.c drv_print.c drv_timer.c drv_l2cache.c drv_sys_cfg.c drv_cpr.c svu_utils.c
#SysASrcFiles     = crt0.S traps.S sysbss.S

#TEST_TYPE     := AUTO
#ComponentList := UnitTest

ComponentList :=

# ------------------------------[ Tools ]------------------------------------------#
# Hardcode tool version here if needed, otherwise defaults to Latest
#MV_TOOLS_VERSION =00.50.23.0

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE   =$(MDK_DIRECTORY)/common

# Include the generic Makefile
include generic_custom.mk


# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options

# Extra app related options
CCOPT			+= -DDEBUG
CCOPT           +=
