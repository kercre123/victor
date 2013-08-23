#PC makefile

PLATFORM = $(shell uname)

PC_FOLDERS= leon_code/main_code pc_code kernels $(MDK_DIRECTORY)/common/swCommon/pcModel $(MDK_DIRECTORY)/common/swCommon/shave_code/myriad1/include $(MDK_DIRECTORY)/common/shared/include shave_code .

PC_SOURCES = shave_code/ImageKernelProcessing.cpp leon_code/main_code/main_code.c \
             pc_code/pcKernelProcessing.cpp\
             kernels/gaussVx2Shave.cpp kernels/gaussHx2Shave.cpp $(MDK_DIRECTORY)/common/swCommon/pcModel/myriadModel.cpp 


			 
ifeq ($(PLATFORM),Linux)
Kern_Executable = output/$(APPNAME)
else
Kern_Executable = output/$(APPNAME).exe
endif

PCCPP = g++
CPP_INCLUDE += $(patsubst %,-I%,$(PC_FOLDERS))
EXTRA_FLAGS = -D_WIN32

CFLAGS=-g -Wall -DPC_MODEL $(CPP_INCLUDE) $(EXTRA_FLAGS)

PCCompile: PCclean
	@echo Building the app ...
	$(PCCPP) $(CFLAGS) $(PC_SOURCES) -o output/$(APPNAME)

PCclean:
	rm -rf $(Kern_Executable)
	rm -rf ./testStreams/DunLoghaire_160x120_P420.yuv

	#./output/$(APPNAME) [input stream] [input_width] [input_height] [number of input frames] [output stream name]
PCRun:
	./output/$(APPNAME) ./testStreams/DunLoghaire_320x240_P420.yuv 320 240 1 ./testStreams/DunLoghaire_160x120_P420.yuv
