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

#This line we could delete once we have all component code in myriad1/myriad2/shared
LEON_COMPONENT_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*,$(ComponentList))

LEON_COMPONENT_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/$(MV_SOC_PLATFORM)/%/*,$(ComponentList))
LEON_COMPONENT_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/shared/%/*,$(ComponentList))

SH_COMPONENTS_PATHS  = $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*,   $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*/*shave*,   $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shave*/*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shared*/*, $(ComponentList))
SH_COMPONENTS_PATHS += $(patsubst %, $(MV_COMMON_BASE)/components/%/*shared*, $(ComponentList))

SH_COMPONENTS_HEADERS_PATHS  = $(patsubst %,$(MV_COMMON_BASE)/components/%/*include*,$(ComponentList))
SH_COMPONENTS_HEADERS_PATHS += $(patsubst %,$(MV_COMMON_BASE)/components/%/*,$(ComponentList))

###################################################################
#  Configuration of Leon(OS) Source Files
###################################################################

# Application Specific Stuff 
ifeq ($(MV_SOC_OS), none)
LEON_APP_C_SOURCES ?=$(wildcard $(DirAppRoot)/*leon/*.c) \
                     $(wildcard $(DirAppRoot)/*leon/*/*.c)

LEON_APP_ASM_SOURCES = $(wildcard $(DirAppRoot)/leon/*.S) \
		       $(wildcard $(DirAppRoot)/leon/*/*.S)
      
# Shared Sources 
LEON_LIB_C_SOURCES ?=$(wildcard $(MV_LEON_LIBC_BASE)/src/*.c)

LEON_COMPONENT_C_SOURCES = $(sort $(wildcard $(patsubst %,%/*.c,$(LEON_COMPONENT_PATHS))))
		
LEON_DRIVER_C_SOURCES  = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/*/*.c \
                         $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/*.c \
                         $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/*.c \
                         $(MV_SWCOMMON_IC)/src/*.c \
                         $(MV_SWCOMMON_BASE)/src/*.c) 

# Driver Assembly Files
LEON_DRIVER_ASM_SOURCES  = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/*.S)
LEON_DRIVER_ASM_SOURCES += $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/asm/traps/traps_$(MV_LEON_TRAP_TYPE).S
LEON_DRIVER_ASM_SOURCES += $(wildcard $(MV_LEON_LIBC_BASE)/src/asm/*.S)
endif

ifeq ($(MV_SOC_OS), rtems)
LEON_APP_C_SOURCES ?=$(wildcard $(DirAppRoot)/*rtems*/*.c) \
                     $(wildcard $(DirAppRoot)/*rtems*/*/*.c)

LEON_DRIVER_ASM_SOURCES  = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/system/rtems_asm/*.S)
#and add the ones from our project by default
LEON_ASM_SOURCES += $(wildcard $(DirAppRoot)/*rtems*/*.S)
LEON_ASM_SOURCES += $(wildcard $(DirAppRoot)/*rtems*/*/*.S)                 
                            
LEON_DRIVER_C_SOURCES ?=
                            
LEON_COMPONENT_C_SOURCES ?=
endif
				 
###################################################################
#  Declaring the Application sources for Leon RT (Myriad2 Only)
###################################################################

LEON_RT_LIB_NAME       	= leon_rt/leonRTApp
LEON_RT_APP_LIBS        = $(LEON_RT_LIB_NAME).mvlib
LEON_RT_C_SOURCES   	= $(wildcard $(DirAppRoot)/leon_rt/*.c)
LEON_RT_C_SOURCES      += $(wildcard $(DirAppRoot)/leon_rt/*/*.c)
LEON_RT_ASM_SOURCES 	= $(wildcard $(DirAppRoot)/leon_rt/*.S) 
LEON_RT_ASM_SOURCES    += $(wildcard $(DirAppRoot)/leon_rt/*/*.S)
LEON_RT_APP_OBJS       	= $(patsubst %.c,%.o,$(LEON_RT_C_SOURCES))
LEON_RT_APP_OBJS       += $(patsubst %.S,%.o,$(LEON_RT_ASM_SOURCES))
		 
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
ifeq ($(MV_SOC_OS), none)
LEON_COMPONENT_HEADERS  = $(wildcard $(patsubst %,%/*.h,$(LEON_COMPONENT_PATHS)))
LEON_COMPONENT_HEADERS += $(wildcard $(patsubst %,%/*.h,$(LEON_COMPONENT_HEADERS_PATHS)))

LEON_HEADERS = $(wildcard $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/brdDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/icDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include/*.h \
                            $(MV_DRIVERS_BASE)/$(MV_SOC_PLATFORM)/socDrivers/include/sysClkCfg/*.h \
                            $(MV_SWCOMMON_IC)/src/*.h \
                            $(MV_SWCOMMON_BASE)/src/*.h \
                            $(MV_LEON_LIBC_BASE)/include)

#and add the ones from our project by default
LEON_HEADERS += $(wildcard $(DirAppRoot)/*leon*/*.h)
LEON_HEADERS += $(LEON_COMPONENT_HEADERS)
endif

ifeq ($(MV_SOC_OS), rtems)
  #Add the ones from our project by default
  LEON_HEADERS += $(wildcard $(DirAppRoot)/*rtems*/*.h)
  LEON_HEADERS += $(wildcard $(DirAppRoot)/*rtems*/*/*.h)
endif

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
# Leon Object File declarations
###################################################################

LEON_SHARED_OBJECTS_REQUIRED  = $(patsubst %.c,%.o,$(LEON_LIB_C_SOURCES))
LEON_SHARED_OBJECTS_REQUIRED += $(patsubst %.c,%.o,$(LEON_COMPONENT_C_SOURCES))
LEON_SHARED_OBJECTS_REQUIRED += $(patsubst %.c,%.o,$(LEON_DRIVER_C_SOURCES))
LEON_SHARED_OBJECTS_REQUIRED += $(patsubst %.S,%.o,$(LEON_DRIVER_ASM_SOURCES))
#and also add the projects raw data files
LEON_SHARED_OBJECTS_REQUIRED += $(RAWDATAOBJECTFILES)

LEON_APP_OBJECTS_REQUIRED    = $(patsubst %.c,%.o,$(LEON_APP_C_SOURCES))
LEON_APP_OBJECTS_REQUIRED   += $(patsubst %.S,%.o,$(LEON_APP_ASM_SOURCES))

ALL_SHAVE_APPS = $(SHAVE0_APPS) $(SHAVE1_APPS) $(SHAVE2_APPS) $(SHAVE3_APPS) $(SHAVE4_APPS) $(SHAVE5_APPS) $(SHAVE6_APPS) $(SHAVE7_APPS) $(SHAVE8_APPS)  $(SHAVE9_APPS)  $(SHAVE10_APPS)  $(SHAVE11_APPS) 

