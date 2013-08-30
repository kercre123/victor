# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))
APPNAME=benchmarking

MDK_DIRECTORY=	/home/anki/movidius/mdk_13.08.0_general/mdk

CORETECH_COMMON_DIRECTORY=../../../coretech-common
CORETECH_VISION_DIRECTORY=../../../coretech-vision

MVCC_OPT_LEVEL = -O1

# Verbose or not, common out for more verbosity
#ECHO=@


# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk
#ShaveSymObject   = $(DirAppObjDir)/$(APPNAME).sym.o
#LinkerScript    =  $(DirLDScript)/$(APPNAME).ldscript
LinkerScript    =  $(DirLDScript)/benchmarking.ldscript

# Set Shave common mof name (non-mbin flow)
#ShaveCommonName = shaves

SH0_Obj += output/obj/shaveMain.mobj output/obj/allFunctions.mobj
ShaveHeaderFiles += $(wildcard shave/)
ShaveCSrcFiles += shave/shaveMain.c shave/allFunctions.c

#SH0_Obj += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(CORETECH_COMMON_DIRECTORY)/src/embedded/*.cpp))))
#SH0_Obj += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(CORETECH_VISION_DIRECTORY)/src/embedded/*.cpp))))

#SH0_Obj += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(CORETECH_COMMON_DIRECTORY)/test/embedded/*.cpp))))
#SH0_Obj += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(CORETECH_VISION_DIRECTORY)/test/embedded/*.cpp))))

#ShaveHeaderFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/include/)
#ShaveHeaderFiles += $(wildcard $(CORETECH_VISION_DIRECTORY)/include/)


#ShaveCPPSrcFiles += $(wildcard $(DirAppObjDir)/shave/*.cpp)
#ShaveCPPSrcFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/src/embedded/*.cpp)
#ShaveCPPSrcFiles += $(wildcard $(CORETECH_VISION_DIRECTORY)/src/embedded/*.cpp)

#ShaveCPPSrcFiles += $(wildcard $(CORETECH_COMMON_DIRECTORY)/test/embedded/*.cpp)
#ShaveCPPSrcFiles += $(wildcard $(CORETECH_VISION_DIRECTORY)/test/embedded/*.cpp)

# Limit to only needed drivers
#SysCSrcFiles     = drv_ddr.c drv_uart.c drv_vcs.c drv_print.c drv_timer.c drv_l2cache.c drv_sys_cfg.c drv_cpr.c svu_utils.c
#SysASrcFiles     = crt0.S traps.S sysbss.S

#TEST_TYPE     := AUTO
#ComponentList := UnitTest

ComponentList := coretech-common

# ------------------------------[ Tools ]------------------------------------------#
# Hardcode tool version here if needed, otherwise defaults to Latest
#MV_TOOLS_VERSION =00.50.23.0

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE   =$(MDK_DIRECTORY)/common

# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk


# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options

# Extra app related options
CCOPT			+= -DDEBUG
CCOPT           +=
