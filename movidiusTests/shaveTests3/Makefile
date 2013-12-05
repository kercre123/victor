# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))
APPNAME=shaveTests2

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk

MDK_DIRECTORY?=../../../movidius-tools/mdk

MYRIAD_BUILDFILE_DIRECTORY?=../../../coretech-common/project/myriad1/

MVCC_OPT_LEVEL ?= -O1
LEON_C_OPTIMIZATION_LEVEL ?= -Os
LEON_CPP_OPTIMIZATION_LEVEL ?= -O2

#Choosing C sources the shaveTests application on shave
ShaveTestsApp = shave/shaveTests
SHAVE_C_SOURCES_shaveTests = $(wildcard $(DirAppRoot)/shave/*.c)
#Choosing ASM sources for the shave shaveTests app on shave
SHAVE_ASM_SOURCES_shaveTests = $(wildcard $(DirAppRoot)/shave/*.asm)

#Generating list of required generated assembly files for the shaveTests app on shave
SHAVE_GENASMS_shaveTests = $(patsubst %.c,%.asmgen,$(SHAVE_C_SOURCES_shaveTests))
#Generating required objects list from sources
SHAVE_shaveTests_OBJS = $(patsubst %.asm,%_shave.o,$(SHAVE_ASM_SOURCES_shaveTests)) \
                   $(patsubst %.asmgen,%_shave.o,$(SHAVE_GENASMS_shaveTests))

#update clean rules with our generated files
PROJECTCLEAN += $(SHAVE_GENASMS_shaveTests) $(SHAVE_shaveTests_OBJS)
#Uncomment below to reject generated shave as intermediary files (consider them secondary)
PROJECTINTERM += $(SHAVE_GENASMS_shaveTests)

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
include $(MYRIAD_BUILDFILE_DIRECTORY)/generic_custom.mk

#-------------------------------[ Local applications build rules ]------------------#
#Describe the rule for building the shaveTestsApp application. Simple rule specifying 
#which objects build up the said application. The application will be built into a library
$(ShaveTestsApp).mvlib : $(SHAVE_shaveTests_OBJS) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) -r $(SHAVE_shaveTests_OBJS) $(PROJECT_SHAVE_LIBS) -o $@


# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options 

# Extra app related options
#CCOPT			+= -DDEBUG
CCOPT           += 


