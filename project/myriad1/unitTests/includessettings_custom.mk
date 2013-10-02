###################################################################
#       Building up list of Leon low level include folders        #
###################################################################
CC_INCLUDE       = $(patsubst %,-I%,$(wildcard    $(DirLeonInclude) \
                                                  $(MV_DRIVERS_BASE)/brdDrivers/include \
                                                  $(MV_DRIVERS_BASE)/icDrivers/include \
                                                  $(MV_COMMON_BASE)/libc/leon/include \
                                                  $(MV_SWCOMMON_BASE)/include \
                                                  $(MV_SWCOMMON_IC)/include \
                                                  $(MV_SHARED_BASE)/include \
                                                  $(MV_SWCOMMON_BASE)/shave_code/$(MV_SOC_PLATFORM)/include \
                                                  $(MV_TOOLS_BASE)/$(DETECTED_PLATFORM)/$(SPARC_DIR)/lib/gcc/sparc-elf/$(GCCVERSION)/include)) 

CC_INCLUDE      += $(patsubst %,-I%,$(wildcard    $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include \
                                                  $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/include \
                                                  $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/include \
                                                  $(DirAppRoot)/leon \
                                                  $(DirAppRoot)/shared))

#And Leon include dirs
LEON_INCLUDE_PATH   += $(patsubst %/,%,$(sort $(dir $(LEON_HEADERS))))
#And blindly add all component paths
LEON_INCLUDE_PATH += $(patsubst %,$(MV_COMMON_BASE)/components/%,$(ComponentList))

CC_INCLUDE += $(patsubst %,-I%,$(wildcard    $(LEON_INCLUDE_PATH)))

#Add the include folders to the CCOPT variable
CCOPT += $(CC_INCLUDE)
CPPOPT += $(CC_INCLUDE)

###################################################################
#       Building up list of Shave assembly include folders        #
###################################################################

MVASM_INCLUDE      = $(patsubst %,-i:%,$(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/include \
                                                  $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/asm)) 
#Adding common folder includes
MVASMOPT		 += $(MVASM_INCLUDE)

###################################################################
#       Building up list of Shave C/C++ include folders           #
###################################################################

MVCC_INCLUDE      = $(patsubst %,-I %,$(wildcard   $(MV_TOOLS_BASE)/common/moviCompile/include ) \
                                                   $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/include \
                                                   $(MV_SHARED_BASE)/include \
                                                   $(DirAppRoot)/shave \
                                                   $(DirAppRoot)/shared)

#And Components include directories
SHAVE_COMPONENT_INCLUDE_PATHS  = $(patsubst %/,%,$(sort $(dir $(SHAVE_COMPONENT_HEADERS))))
#And blindly add all component paths
SHAVE_COMPONENT_INCLUDE_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%,$(ComponentList))

SHAVE_APP_HEADER_PATHS = $(patsubst %/,%,$(sort $(dir $(SHAVE_HEADERS))))

MVCC_INCLUDE += $(patsubst %,-I %,$(sort $(SHAVE_COMPONENT_INCLUDE_PATHS) $(SHAVE_APP_HEADER_PATHS)))

MVCCOPT += $(MVCC_INCLUDE)
