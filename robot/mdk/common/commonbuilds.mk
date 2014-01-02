#commonbuilds.mk - generic.mk addition to be used with the Movidius elf build flow
#                  in order to add build rules for commonly used libraries
#
# Created on: Jul 17, 2013
#     Author: Cristian-Gavril Olar

#----------------------[ Software common library files ]---------------------------#

SH_SWCOMMON_C_SOURCES = $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.c)
SH_SWCOMMON_ASM_SOURCES = $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.asm)
SH_SWCOMMON_CPP_SOURCES = $(wildcard $(MV_COMMON_BASE)/swCommon/shave_code/$(MV_SOC_PLATFORM)/*/*.cpp)

SH_SWCOMMON_GENASMS  = $(patsubst %.c,%.asmgen,$(SH_SWCOMMON_C_SOURCES))
SH_SWCOMMON_GENASMS += $(patsubst %.c,%.asmgen,$(SH_SWCOMMON_CPP_SOURCES))

SH_SWCOMMON_OBJS  = $(patsubst %.asm,%_shave.o,$(SH_SWCOMMON_ASM_SOURCES))
SH_SWCOMMON_OBJS += $(patsubst %.asmgen,%_shave.o,$(SH_SWCOMMON_GENASMS))

SH_SWCOMMON_MBINOBJS  = $(patsubst %.asm,%.mobj,$(SH_SWCOMMON_ASM_SOURCES))
SH_SWCOMMON_MBINOBJS += $(patsubst %.asmgen,%.mobj,$(SH_SWCOMMON_GENASMS))

#----------------------[ Components library files ]---------------------------#
SHAVE_COMPONENT_C_SOURCES = $(sort $(wildcard $(patsubst %,%/*.c,$(SH_COMPONENTS_PATHS))))
SHAVE_COMPONENT_CPP_SOURCES = $(sort $(wildcard $(patsubst %,%/*.cpp,$(SH_COMPONENTS_PATHS))))
SHAVE_COMPONENT_ASM_SOURCES = $(sort $(wildcard $(patsubst %,%/*.asm,$(SH_COMPONENTS_PATHS))))

SHAVE_COMPONENT_GENASMS  = $(patsubst %.c,%.asmgen,$(SHAVE_COMPONENT_C_SOURCES))
SHAVE_COMPONENT_GENASMS += $(patsubst %.cpp,%.asmgen,$(SHAVE_COMPONENT_CPP_SOURCES))

SHAVE_COMPONENT_OBJS  = $(patsubst %.asm,%_shave.o,$(SHAVE_COMPONENT_ASM_SOURCES))
SHAVE_COMPONENT_OBJS += $(patsubst %.asmgen,%_shave.o,$(SHAVE_COMPONENT_GENASMS))

#----------------------[ Actual build rules ]---------------------------#
# swCommon mlib
$(CommonMlibFile).mvlib: $(SH_SWCOMMON_OBJS)
	@echo "Shave Common LIB      : $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(LD) $(LD_ENDIAN_OPT) -r $(SH_SWCOMMON_OBJS) -o $@ 

$(CommonMlibFile).mlib: $(SH_SWCOMMON_MBINOBJS)
	@echo "Shave Common LIB (mbin): $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVLNK) -cv:$(MV_SOC_PLATFORM) -pad:16 $^ -o:$(CommonMlibFile).mlib -lib  $(DUMP_NULL)

#The apps own lib
$(ProjectShaveLib) : $(SHAVE_COMPONENT_OBJS)
	@echo "Shave Component LIB   : $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(LD) $(LD_ENDIAN_OPT) -r $(SHAVE_COMPONENT_OBJS) -o $@

#---------------------[ Listing file builds ]----------------------------#
# These are not files built for executing however they are "common" and
# not related to the build flow. So instead of creating a new *.mk file
# or poluting the generik.mk file that contains build flow rules
# I'm adding these targets here

.INTERMEDIATE: $(DirAppOutput)/lst/$(APPNAME).elf.nodebug

# Generating a binutils generated version of the project
# This file contains valid leon disassembly, but doesn't decode SHAVE sections
$(DirAppOutput)/lst/$(APPNAME)_leon.lst : $(DirAppOutput)/lst/$(APPNAME).elf.nodebug
	@echo "Leon Dbg Listing      : $(DirAppOutput)/lst/$(APPNAME)_leon.lst"
	$(ECHO) $(OBJDUMP) -xsrtd $< > $@

# Movidius Object dump of project elf file 
# This file contains full disasembly of leon and shave sections 
$(DirAppOutput)/lst/$(APPNAME)_shave.lst : $(DirAppOutput)/lst/$(APPNAME).elf.nodebug
	@echo "Shave Dbg Listing     : $(DirAppOutput)/lst/$(APPNAME)_shave.lst"
	@# Temporarily suppress errors until this version of moviDump is officially released
	-$(ECHO) $(MVDUMP) -cv:$(MV_SOC_PLATFORM) $< -o:$@  $(DUMP_NULL)
			    
# Intermediate copy of the elf file with the key debug sections removed
# Manually selecting the debug sections to remove as I found the --strip-debug
# option too agressive
# This intermediate target is automatically deleted when make completes
$(DirAppOutput)/lst/$(APPNAME).elf.nodebug : $(DirAppOutput)/$(APPNAME).elf
	@mkdir -p $(DirAppOutput)/lst
	$(ECHO) $(OBJCOPY) -R .debug_aranges \
			   -R .debug_ranges \
			   -R .debug_pubnames \
			   -R .debug_info \
			   -R .debug_abbrev \
			   -R .debug_asmline \
			   -R .debug_line \
			   -R .debug_str \
			   -R .debug_loc \
			   -R .debug_macinfo \
			$< $@

