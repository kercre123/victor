###################################################################
#       Building up list of components paths                      #
###################################################################

#Build the list of leon source required from the components variable

LEON_COMPONENT_PATHS  = $(patsubst %,$(MV_COMMON_BASE)/components/%/*leon*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*/*leon*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*leon*/*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*leon*/*/*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*leon*/*/*/*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*shared*/*,$(ComponentList))
LEON_COMPONENT_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*shared*,$(ComponentList))

LEON_COMPONENT_HEADERS_PATHS  = $(patsubst %,$(MV_COMMON_BASE)/components/%/*include*,$(ComponentList))
LEON_COMPONENT_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*,$(ComponentList))

SH_COMPONENTS_PATHS  = $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*,   $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*/*shave*,   $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shared*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shared*, $(ComponentList))

SH_COMPONENTS_HEADERS_PATHS  = $(patsubst %,$(MV_COMMON_BASE)/components/%/*include*,$(ComponentList))
SH_COMPONENTS_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*,$(ComponentList))

###################################################################
#       Building up list of Leon low level driver c files         #
###################################################################

LEON_C_SOURCES ?=$(wildcard $(DirAppRoot)/*leon*/*.c) \
                 $(wildcard $(DirAppRoot)/*leon*/*/*.c)

LEON_C_SOURCES += $(wildcard $(MV_LEON_LIBC_BASE)/src/*.c)

LEON_CPP_SOURCES ?=

                            
LEON_DRIVER_C_SOURCES = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/*/*.c \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/*.c \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/*.c \
                            $(MV_SWCOMMON_IC)/src/*.c \
                            $(MV_SWCOMMON_BASE)/src/*.c) 
                            
LEON_COMPONENT_C_SOURCES = $(sort $(wildcard $(patsubst %,%/*.c,$(LEON_COMPONENT_PATHS))))

###################################################################
#  Declaring the SHAVE applications used by the app.              #
###################################################################

SHAVE_APP_LIBS ?=
SHAVE0_APPS ?=
SHAVE1_APPS ?=
SHAVE2_APPS ?=
SHAVE3_APPS ?=
SHAVE4_APPS ?=
SHAVE5_APPS ?=
SHAVE6_APPS ?=
SHAVE7_APPS ?=
                 
PROJECTCLEAN += $(SHAVE_APP_LIBS)

###################################################################
#       Building up list of Leon low level driver h files         #
###################################################################

LEON_COMPONENT_HEADERS  = $(wildcard $(patsubst %,%/*.h,$(LEON_COMPONENT_PATHS)))
LEON_COMPONENT_HEADERS += $(wildcard $(patsubst %,%/*.h,$(LEON_COMPONENT_HEADERS_PATHS)))

LEON_HEADERS ?=
LEON_HEADERS += $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include/sysClkCfg/*.h \
                            $(MV_SWCOMMON_IC)/src/*.h \
                            $(MV_SWCOMMON_BASE)/src/*.h \
                            $(MV_LEON_LIBC_BASE)/include)

#and add the ones from our project by default
LEON_HEADERS += $(wildcard $(DirAppRoot)/*leon*/*.h)
LEON_HEADERS += $(LEON_COMPONENT_HEADERS)  

###################################################################
#       Building up list of Shave low level driver incl files     #
###################################################################

SHAVE_ASM_HEADERS = $(wildcard $(MV_SWCOMMON_BASE)/shave_code/$(MV_SOC_PLATFORM)/include/*.incl)
#and add the ones from our project by default
SHAVE_ASM_HEADERS += $(wildcard $(DirAppRoot)/shave/*.incl)

###################################################################
#       Building up list of Shave low level driver header files   #
###################################################################
SHAVE_HEADERS  = $(wildcard $(DirAppRoot)/*shave*/*.h)
SHAVE_HEADERS += $(wildcard $(DirAppRoot)/*shave*/*.hpp)

#and the headers from components
SHAVE_COMPONENT_HEADERS  = $(wildcard $(patsubst %,%/*.h,$(SH_COMPONENTS_PATHS)))
SHAVE_COMPONENT_HEADERS += $(wildcard $(patsubst %,%/*.h,$(SH_COMPONENTS_HEADERS_PATHS)))

###################################################################
#       Building up list of Leon low level driver assembly files  #
###################################################################

LEON_DRIVER_ASM_SOURCES  = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/*.S)
LEON_DRIVER_ASM_SOURCES += $(wildcard $(MV_LEON_LIBC_BASE)/src/asm/*.S)
#and add the ones from our project by default
LEON_ASM_SOURCES += $(wildcard $(DirAppRoot)/leon/*.S)

###################################################################
#       Building up list of Leon low level driver object files    #
###################################################################

LEON_DRIVER_OBJECTS_REQUIRED = $(patsubst %.c,%.o,$(LEON_DRIVER_C_SOURCES))
LEON_DRIVER_OBJECTS_REQUIRED += $(patsubst %.S,%.o,$(LEON_DRIVER_ASM_SOURCES))

LEON_OBJECTS_REQUIRED += $(patsubst %.c,%.o,$(LEON_C_SOURCES))
LEON_OBJECTS_REQUIRED += $(patsubst %.cpp,%.o,$(LEON_CPP_SOURCES))
LEON_OBJECTS_REQUIRED += $(patsubst %.S,%.o,$(LEON_ASM_SOURCES))
LEON_OBJECTS_REQUIRED += $(patsubst %.c,%.o,$(LEON_COMPONENT_C_SOURCES))
#and also add the projects raw data files
LEON_OBJECTS_REQUIRED += $(RAWDATAOBJECTFILES)

ALL_SHAVE_APPS = $(SHAVE0_APPS) $(SHAVE1_APPS) $(SHAVE2_APPS) $(SHAVE3_APPS) $(SHAVE4_APPS) $(SHAVE5_APPS) $(SHAVE6_APPS) $(SHAVE7_APPS)
