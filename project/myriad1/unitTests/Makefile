# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))
APPNAME=coretech-common-unit-tests

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk

MDK_DIRECTORY?=../../../../movidius-tools/mdk

CORETECH_COMMON_DIRECTORY?=../../../../coretech-common

MVCC_OPT_LEVEL ?= -O1
LEON_C_OPTIMIZATION_LEVEL ?= -Os
LEON_CPP_OPTIMIZATION_LEVEL ?= -O3

SH0_Obj += output/obj/shaveMain.mobj

# ShaveCSrcFiles += shave/shaveMain.c 

LEON_C_SOURCES += leon/main.c
LEON_C_SOURCES += leon/app_config.c

#LEON_CPP_SOURCES += leon/cppInterface.cpp

LEON_HEADERS += $(wildcard $(CORETECH_COMMON_DIRECTORY)/include/)

#LEON_CPP_SOURCES += $(wildcard $(CORETECH_COMMON_DIRECTORY)/src/embedded/*.cpp)
#LEON_C_SOURCES += $(wildcard $(CORETECH_COMMON_DIRECTORY)/src/embedded/*.c)

#LEON_CPP_SOURCES += $(wildcard $(CORETECH_COMMON_DIRECTORY)/test/embedded/embeddedCommonTests.cpp)

# ------------------------------[ Components used ]--------------------------------#
ComponentList = Board

#-------------------------------[ Local shave applications sources ]--------------------#
#No local shave applications for this example

#Include apps-to-shaves mapping configuration file
include ./config/config_shaves.mk

# ------------------------------[ Tools overrides ]--------------------------------#
# Hardcode tool version here if needed, otherwise defaults to latest stable release
#MV_TOOLS_VERSION =00.50.23.0

# Verbose or not, uncomment for more verbosity
#ECHO=

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE ?= $(MDK_DIRECTORY)/common

# Include the generic Makefile
# include $(MV_COMMON_BASE)/generic.mk
include generic_custom.mk

#-------------------------------[ Local shave applications build rules ]------------------#
#No local applications build rules for this example


# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options 

# Extra app related options
#CCOPT			+= -DDEBUG
CCOPT           += 


