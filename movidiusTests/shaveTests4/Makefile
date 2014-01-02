# ------------------------------[ App ]--------------------------------------------#
# Default application name is the same as the folder it is in.
# This can be overridden here if something different is required
#APPNAME ?= $(notdir $(shell pwd))

# ------------------------------[ Build overrides ]--------------------------------#
# overrides that need to be set before including generic.mk
MV_SOC_PLATFORM = myriad1

MDK_DIRECTORY?=../../../movidius-tools/mdk

MYRIAD_BUILDFILE_DIRECTORY?=.

# ------------------------------[ Components used ]--------------------------------#
#ComponentList = 
#No components used by 001_HelloWorldShave
#Choosing if this project has shave components or not
SHAVE_COMPONENTS = no

#-------------------------------[ Settings ]---------------------------------------#

#-------------------------------[ Local shave applications sources ]--------------------#

#Choosing C sources the helloShave application on shave
HelloShaveApp = shave/helloShave
SHAVE_C_SOURCES_helloShave = $(wildcard $(DirAppRoot)/shave/*.c)

#Choosing ASM sources for the shave helloShave app on shave
#SHAVE_ASM_SOURCES_helloShave =

#Generating list of required generated assembly files for the helloShave app on shave
SHAVE_GENASMS_helloShave = $(patsubst %.c,%.asmgen,$(SHAVE_C_SOURCES_helloShave))

#Generating required objects list from sources
SHAVE_helloShave_OBJS = $(patsubst %.asmgen,%_shave.o,$(SHAVE_GENASMS_helloShave))

#update clean rules with our generated files
PROJECTCLEAN += $(SHAVE_GENASMS_helloShave) $(SHAVE_helloShave_OBJS)

#Uncomment below to reject generated shave as intermediary files (consider them secondary)
PROJECTINTERM += $(SHAVE_GENASMS_helloShave)

#--------------------------[ Shave applications section ]--------------------------#
SHAVE_APP_LIBS = $(HelloShaveApp).mvlib
SHAVE0_APPS = $(HelloShaveApp).shv0lib

# ------------------------------[ Tools overrides ]--------------------------------#
# Hardcode tool version here if needed, otherwise defaults to latest stable release
#MV_TOOLS_VERSION =00.50.23.0

# Verbose or not, uncomment for more verbosity
#ECHO=

# Set MV_COMMON_BASE relative to mdk directory location (but allow user to override in environment)
MV_COMMON_BASE ?= $(MDK_DIRECTORY)/common

# Include the generic Makefile
include $(MV_COMMON_BASE)/generic.mk

#-------------------------------[ Local shave applications build rules ]------------------#
#Describe the rule for building the 001_HelloWorldShave application. Simple rule specifying 
#which objects build up the said application. The application will be built into a library
$(HelloShaveApp).mvlib : $(SHAVE_helloShave_OBJS) $(PROJECT_SHAVE_LIBS)
	$(ECHO) $(LD) -r $(LD_ENDIAN_OPT) $(SHAVE_helloShave_OBJS) $(PROJECT_SHAVE_LIBS) -o $@


# -------------------------------- [ Build Options ] ------------------------------ #
# App related build options 

# Extra app related options
#CCOPT			+= -DDEBUG
CCOPT           += 

ifeq ($(MV_SOC_PLATFORM),myriad1)
MVCCOPT += -DMYRIAD1
CCOPT += -DMYRIAD1
endif
