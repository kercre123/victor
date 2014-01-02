APPDIR=$(shell pwd)
APPNAME ?= $(shell basename '$(APPDIR)')

MV_COMMON_BASE   ?= ../../../../../common
MDK_BASE ?= ../../../../..
UNIT_TEST_DIR = $(MDK_BASE)/regression/unittest
MV_EXTRA_DATA = $(MV_COMMON_BASE)/../../mv_extra_data

KERNEL_APP_NAME = medianFilter15x15
KERNEL_APP_PATH = $(ASM_KERNEL_TEST_DIR)/$(KERNEL_APP_NAME)

CPPFLAGS += -D"APP_PATH=\"$(KERNEL_APP_PATH)\""
CPPFLAGS += -D"APP_ELFPATH=\"$(KERNEL_APP_PATH)/output/$(KERNEL_APP_NAME).elf\""
#set the flag below DLL_DBGI_JTAG for jtag interface or DLL_DBGI_MSIM for simulator
CPPFLAGS += -DDBG_INTERFACE=DLL_DBGI_JTAG
#shave prefix used to create shave symbol names
CPPFLAGS += -D"SHAVE_NAME_PREFIX=\"medianFilter15x15\""
TEST_NAME  = medianFilter15x15_unittest

include $(UNIT_TEST_DIR)/unittest.mk

ifeq ($(PLATFORM),Linux)
Test_Executable = $(TEST_NAME)
else
Test_Executable = $(TEST_NAME).exe
endif

B_DATA_LIST = resources/g_test/gtest-1.6.0
include $(MV_COMMON_BASE)/bigdata.mk
    
run_test:
	cd $(KERNEL_APP_PATH); make all
	./run/$(Test_Executable)