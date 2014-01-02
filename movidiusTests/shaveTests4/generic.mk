#generic.mk - simple generic.mk to be used with the Movidius one stage build flow
#
# Created on: Jul 17, 2013
#     Author: Cristian-Gavril Olar

#Need some tentative definitions first and we need to detect the tools to be used

###################################################################
#       Using settings makefiles for wider system settings        #
###################################################################
include $(MV_COMMON_BASE)/generalsettings.mk
include $(MV_COMMON_BASE)/toolssettings.mk
include $(MV_COMMON_BASE)/includessettings.mk
include $(MV_COMMON_BASE)/commonsources.mk
###################################################################
#       And finally listing all of the build rules                #
###################################################################

#For external users we have an extra check here that we must do to ensure they have MV_TOOLS_DIR set
#as environment variable
ifeq ($(CHECK_TOOLS_VAR),yes)
CHECKINSTALLOUTPUT=$(shell cd $(MV_COMMON_BASE)/utils && ./mincheck.sh)

all:
	@echo $(CHECKINSTALLOUTPUT)
ifeq ($(CHECKINSTALLOUTPUT),MV_TOOLS_DIR set)
	+make trueall
endif
#Output files and listings files
trueall : $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(DirAppOutput)/lst/$(APPNAME)_leon.lst $(DirAppOutput)/lst/$(APPNAME)_shave.lst  
else
#Output files and listings files
all : $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(DirAppOutput)/lst/$(APPNAME)_leon.lst $(DirAppOutput)/lst/$(APPNAME)_shave.lst 
endif

$(DirAppOutput)/$(APPNAME).map : $(DirAppOutput)/$(APPNAME).elf

$(DirAppOutput)/$(APPNAME).elf :  $(LEON_RT_APPS) $(LEON_APP_OBJECTS_REQUIRED) $(LEON_SHARED_OBJECTS_REQUIRED) $(ALL_SHAVE_APPS) $(AllLibs) $(LinkerScript) 
	@echo "Application ELF       : $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(LD) $(LDOPT) -o $@ $(LEON_RT_APPS) $(LEON_SHARED_OBJECTS_REQUIRED) $(LEON_APP_OBJECTS_REQUIRED) $(AllLibs) $(ALL_SHAVE_APPS) $(DefaultSparcRTEMSLibs) $(DefaultSparcGccLibs) > $(DirAppOutput)/$(APPNAME).map
	@echo "Application MAP File  : $(DirAppOutput)/$(APPNAME).map"

%.o : %.c $(LEON_HEADERS) Makefile
	@echo "Leon CC   : $<"
	$(ECHO) $(CC) -c $(CCOPT) $< -o $@

%.o : %.S $(LEON_HEADERS) Makefile
	@echo "Leon ASM  :  $<"
	$(ECHO) $(CC) -c $(CCOPT) -DASM $< -o $@

ifeq ($(SHAVEDEBUG),yes)
# Temporarily need to add the debug information by hand
%_shave.o : %.asmgen $(SHAVE_ASM_HEADERS) Makefile
	@echo "Shave ASM : $<"
	$(ECHO) $(MVASM) $(MVASMOPT) -dwarf:$(notdir $(patsubst %.asmgen,%.elf,$<)) -elf $< -o:$@ $(DUMP_NULL)
	$(ECHO) rm -rf $(notdir $(patsubst %.asmgen,%.elf,$<))
else
%_shave.o : %.asmgen $(SHAVE_ASM_HEADERS) Makefile
	@echo "Shave ASM : $<"
	$(ECHO) $(MVASM) $(MVASMOPT) -elf $< -o:$@ $(DUMP_NULL)
endif

%_shave.o : %.asm $(SHAVE_ASM_HEADERS) Makefile
	@echo "Shave ASM : $<"
	$(ECHO) $(MVASM) $(MVASMOPT) -elf $< -o:$@  $(DUMP_NULL)

%.asmgen : %.c $(SHAVE_C_HEADERS) Makefile
	@echo "Shave CC  :  $<"
	$(ECHO) $(MVCC) $(MVCCOPT) $< -o $@ $(DUMP_NULL)

%.asmgen : %.cpp $(SHAVE_C_HEADERS) Makefile
	@echo "Shave CPP :  $<"
	$(ECHO) $(MVCC) $(MVCCOPT) $(MVCCPPOPT) $< -o $@  $(DUMP_NULL)

%.i : %.c $(SHAVE_C_HEADERS) Makefile
	@echo "Shave pre-process c   :  $<"
	$(ECHO) $(MVCC) $(MVCCOPT) -E $< -o $@ $(DUMP_NULL)

%.ipp : %.cpp $(SHAVE_C_HEADERS) Makefile
	@echo "Shave pre-process c++ :  $<"
	$(ECHO) $(MVCC) $(MVCCOPT) -E $< -o $@ $(DUMP_NULL)

#Move section prefixes for shave0 selected applications. This is the section describing 
#ABSOLUTE relocations
#That filter line there is just taking out the application name by removing the ".mvlib" extension
#and the leading words  
%.shv0lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv0.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))0_ $< $@
%.shv1lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv1.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))1_ $< $@
%.shv2lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv2.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))2_ $< $@
%.shv3lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv3.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))3_ $< $@
%.shv4lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv4.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))4_ $< $@
%.shv5lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv5.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))5_ $< $@
%.shv6lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv6.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))6_ $< $@
%.shv7lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv7.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))7_ $< $@
%.shv8lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv8.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))8_ $< $@
%.shv9lib : %.mvlib                                        
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv9.  --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))9_ $< $@
%.shv10lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv10. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))10_ $< $@
%.shv11lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv11. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))11_ $< $@

#And one section to place contents in windowed addresses if we so choose
%.shv0wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv0. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))0_ $< $@
%.shv1wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv1. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))1_ $< $@
%.shv2wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv2. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))2_ $< $@
%.shv3wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv3. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))3_ $< $@
%.shv4wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv4. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))4_ $< $@
%.shv5wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv5. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))5_ $< $@
%.shv6wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv6. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))6_ $< $@
%.shv7wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv7. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))7_ $< $@
%.shv8wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv8. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))8_ $< $@
%.shv9wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv9. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))9_ $< $@
%.shv10wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv10. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))10_ $< $@
%.shv11wndlib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.wndshv11. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))11_ $< $@

#Targets for dynamically loadable objects
%.shvdlib : %.mvlib
	@echo "Creating dynamically loadable library : $@"
	$(ECHO) $(LD) $(LD_ENDIAN_OPT) $(LDDYNOPT) $< -o $@

#this gives out all symbols too
%.shvdcomplete : %.mvlib
	@echo "Creating windowed library for symbol extraction $@"
	$(ECHO) $(LD) $(LD_ENDIAN_OPT) $(LDSYMOPT) $< -o $@

#this creates the required symbols file with only the symbols for the dynamic section
%_sym.o : %.shvdcomplete
	@echo "Creating symbols file for the dynamic section $@"
	$(ECHO) $(OBJCOPY) --prefix-symbols=$(lastword $(filter-out shvdcomplete,$(subst /, ,$(subst ., ,$<))))_ --extract-symbol $< $@

#Some generic targets for raw objects
%_raw.o : %.shvdlib
	$(ECHO) $(OBJCOPY)  -I binary --rename-section .data=.ddr_direct.data \
	--redefine-sym  _binary_$(subst /,_,$(subst .,_,$<))_start=$(lastword $(filter-out shvdlib,$(subst /, ,$(subst ., ,$<))))Mbin \
	-O elf32-sparc -B sparc $< $@

# General rule to make MVcmd from a elf
$(DirAppOutput)/%.mvcmd : $(DirAppOutput)/%.elf
	@echo "MVCMD boot image      : $@"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVCONV) -elfInput $(MVCMDOPT) $(^) -mvcmd:$(@)  $(DUMP_NULL)

$(LinkerScript) : $(LinkerScriptSource)
	@echo "Generate Linker Script: $(<F) -> $@"
	$(ECHO) $(CC) -E -P -C -I $(DirAppRoot)/scripts -I $(DirLDScript) -I $(DirLDScrCommon)/$(MV_SOC_PLATFORM) $(LinkerScriptSource) -o $(LinkerScript)

###################################################################
# LeonRT Build Rules (Myriad2 Specific)
###################################################################
		  
$(LEON_RT_LIB_NAME).mvlib : $(LEON_RT_APP_OBJS) $(LEON_SHARED_OBJECTS_REQUIRED)
	$(ECHO) $(LD) $(LD_ENDIAN_OPT) -r $(LEON_RT_APP_OBJS) $(LEON_SHARED_OBJECTS_REQUIRED) -o $@

%.rtlib : %.mvlib
	@echo Generating unique instance of leon_rt application: $@
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.lrt --prefix-symbols=lrt_ $< $@
		  
###################################################################
#       Including some specific build rules we need for all       #
###################################################################
-include $(MV_COMMON_BASE)/bigdata.mk
include $(MV_COMMON_BASE)/commonbuilds.mk
include $(MV_COMMON_BASE)/mbinflow.mk
include $(MV_COMMON_BASE)/functional_targets.mk
-include $(MV_COMMON_BASE)/inhousetargets.mk

localclean:
	@echo "Cleaning project specific built files."
	$(ECHO) rm -rf $(LEON_APP_OBJECTS_REQUIRED) $(LEON_RT_APPS) $(PROJECTCLEAN) $(ALL_SHAVE_APPS) $(SHAVE_APP_LIBS) $(DirAppOutput)/$(APPNAME).map $(CommonMlibFile).mlib $(CommonMlibFile).mvlib $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(DirAppOutput)/lst

clean:
	@echo "Cleaning all built files from the MDK distribution and all project built files."
	$(ECHO) rm -rf  $(SHAVE_COMPONENT_OBJS) $(LEON_SHARED_OBJECTS_REQUIRED) $(LEON_APP_OBJECTS_REQUIRED) $(LEON_RT_APPS) $(SH_SWCOMMON_OBJS) $(SH_SWCOMMON_GENASMS) $(PROJECTCLEAN) $(LEON_OBJECTS_REQUIRED) $(SHAVE_APP_LIBS) $(ALL_SHAVE_APPS) $(DirAppOutput)/$(APPNAME).map $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(CommonMlibFile).mlib $(CommonMlibFile).mvlib $(ProjectShaveLib) $(DirAppOutput)/* 

.INTERMEDIARY: $(SHAVE_GENERATED_ASMS) $(PROJECTINTERM)
