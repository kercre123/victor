APPDIR=$(shell pwd)
APPNAME ?= $(shell basename '$(APPDIR)')

# ----------------------[ MV_SOC_PLATFORM switch ]---------------------------------#
MV_SOC_PLATFORM ?= myriad1
ifeq ($(findstring myriad1, $(MAKECMDGOALS)),myriad1)
MV_SOC_PLATFORM = myriad1
#set the flag below DLL_DBGI_JTAG for jtag interface or DLL_DBGI_MSIM for simulator
CPPFLAGS += -DDBG_INTERFACE=DLL_DBGI_JTAG
else ifeq ($(findstring myriad2, $(MAKECMDGOALS)),myriad2)
MV_SOC_PLATFORM = myriad2
CPPFLAGS += -DDBG_INTERFACE=DLL_DBGI_MSIM
endif
myriad1 myriad2 :
	@echo MV_SOC_PLATFORM = $(MV_SOC_PLATFORM)
# ---------------------------------------------------------------------------------#	

MDK_BASE ?= ../../../../..
MV_COMMON_BASE   ?= $(MDK_BASE)/common
UNIT_TEST_DIR = $(MDK_BASE)/regression/unittest
MV_EXTRA_DATA = $(MV_COMMON_BASE)/../../mv_extra_data

KERNEL_APP_NAME = LookupTable12to8
KERNEL_APP_PATH = $(ASM_KERNEL_TEST_DIR)/$(KERNEL_APP_NAME)
SHAVE_NAME_PREFIX = LookupTable12to8

CPPFLAGS += -D"APP_PATH=\"$(KERNEL_APP_PATH)\""
CPPFLAGS += -D"APP_ELFPATH=\"$(KERNEL_APP_PATH)/output/$(KERNEL_APP_NAME).elf\""

#shave prefix used to create shave symbol names
CPPFLAGS += -D"SHAVE_NAME_PREFIX=\"$(SHAVE_NAME_PREFIX)\""
TEST_NAME  = LookupTable12to8_unittest

include $(UNIT_TEST_DIR)/unittest.mk

ifeq ($(PLATFORM),Linux)
Test_Executable = $(TEST_RUN_DIR)/$(TEST_NAME)
else
Test_Executable = $(TEST_RUN_DIR)/$(TEST_NAME).exe
endif

B_DATA_LIST = resources/g_test/gtest-1.6.0
include $(MV_COMMON_BASE)/bigdata.mk
    
run_test:
	cd $(KERNEL_APP_PATH); make clean $(MV_SOC_PLATFORM) all
	$(Test_Executable)
