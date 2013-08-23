# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))

# Verbose or not, common out for more verbosity
ECHO=@

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk

#ShaveSymObject   = $(DirAppObjDir)/$(APPNAME).sym.o
LinkerScript    =  $(DirLDScript)/$(APPNAME).ldscript
# Set Shave common mof name (non-mbin flow)  
#ShaveCommonName = shaves

SH_MLIB  = $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.c, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvSTL/shave/*.c))))
SH_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvSTL/shave/*.cpp))))
SH_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.asm, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvSTL/shave/asm/*.asm))))

SH_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.c, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvCV/shave/*.c))))
SH_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.cpp, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvCV/shave/*.cpp))))
SH_MLIB += $(addprefix $(DirAppObjDir)/, $(notdir $(patsubst %.asm, %.mobj, $(wildcard $(MV_COMMON_BASE)/components/MvCV/shave/asm/*.asm))))

SH0_Obj =

SH1_Obj += $(DirAppObjDir)/ImageKernelProcessing.mobj
SH1_Obj += 

# Limit to only needed drivers
#SysCSrcFiles     = drv_ddr.c drv_uart.c drv_vcs.c drv_print.c drv_timer.c drv_l2cache.c drv_sys_cfg.c drv_cpr.c svu_utils.c  
#SysASrcFiles     = crt0.S traps.S sysbss.S 

#TEST_TYPE     := AUTO
#ComponentList := UnitTest
ComponentList := MvCV MvSTL CifGeneric Board LcdGeneric HdmiTxIte CifOV5642

# ------------------------------[ Tools ]------------------------------------------#
# Hardcode tool version here if needed, otherwise defaults to Latest
#MV_TOOLS_VERSION =00.50.23.0

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE   ?= ../../common

# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk

# Include the PC cross compile Makefile
include PC_compile.mk
      
# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options 

# Extra app related options
#CCOPT += -DNAME=VALUE
CCOPT +=
MVCCOPT += -cpp
# ------------------------------[ Extra Rules ]-------------------------------------------#

