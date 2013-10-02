#generic.mk - simple generic.mk to be used with the Movidius one stage build flow
#
# Created on: Jul 17, 2013
#     Author: Cristian-Gavril Olar
# Modified on: Oct 2, 2013
#     Author: Peter Barnum

#Need some tentative definitions first and we need to detect the tools to be used

###################################################################
#       Using settings makefiles for wider system settings        #
###################################################################
include generalsettings_custom.mk
include toolssettings_custom.mk
include includessettings_custom.mk
include commonsources_custom.mk
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

$(DirAppOutput)/$(APPNAME).elf : $(LEON_OBJECTS_REQUIRED) $(LEON_DRIVER_OBJECTS_REQUIRED) $(ALL_SHAVE_APPS) $(AllLibs) $(LinkerScript)
	@echo "Linking file $(notdir $@)"
	@mkdir -p $(dir $@)
	@echo $(LD) $(LDOPT) -o $@ $(LEON_DRIVER_OBJECTS_REQUIRED) $(LEON_OBJECTS_REQUIRED) $(AllLibs) $(ALL_SHAVE_APPS) $(DefaultSparcGccLibs) > $(DirAppOutput)/$(APPNAME).map
	$(ECHO) $(LD) $(LDOPT) -o $@ $(LEON_DRIVER_OBJECTS_REQUIRED) $(LEON_OBJECTS_REQUIRED) $(AllLibs) $(ALL_SHAVE_APPS) $(DefaultSparcGccLibs) > $(DirAppOutput)/$(APPNAME).map

%.o : %.c $(LEON_HEADERS) Makefile
	@echo Leon compiling C $(notdir $@)
	@echo $(CC) -c $(CCOPT) $< -o $@
	$(ECHO) $(CC) -c $(CCOPT) $< -o $@

%.o : %.cpp $(LEON_HEADERS) Makefile
	@echo Leon compiling C++ $(notdir $@)
	@echo $(CC) -c $(CPPOPT) $< -o $@
	$(ECHO) $(CC) -c $(CPPOPT) $< -o $@

%.o : %.S $(LEON_HEADERS) Makefile
	@echo Leon assembling Assembly $(notdir $@)
	@echo $(CC) -c $(CCOPT) -DASM $< -o $@
	$(ECHO) $(CC) -c $(CCOPT) -DASM $< -o $@

ifeq ($(SHAVEDEBUG),yes)
#tempoirarily need to add the debug information by hand
%_shave.o : %.asmgen $(SHAVE_ASM_HEADERS) Makefile
	@echo Shave assembling $(notdir $@)
	$(ECHO) $(MVASM) $(MVASMOPT) -dwarf:$(notdir $(patsubst %.asmgen,%.elf,$<)) -elf $< -o:$@
	$(ECHO) rm -rf $(notdir $(patsubst %.asmgen,%.elf,$<))
	@cho Generating listing file for $(notdir $@)
	$(MVASM) $(MVASMOPT) $< -o:$(patsubst %_shave.o,%.mobj,$@)
	$(MVDUMP) -all $(patsubst %_shave.o,%.mobj,$@)
	@mkdir -p $(DirAppOutput)/lst/shave
	mv $(patsubst %_shave.o,%.mobj,$@) $(DirAppOutput)/lst/shave/$(patsubst %_shave.o,%.mobj,$(notdir $@))
else
%_shave.o : %.asmgen $(SHAVE_ASM_HEADERS) Makefile
	@echo Shave assembling $(notdir $@)
	$(ECHO) $(MVASM) $(MVASMOPT) -elf $< -o:$@
	@echo Generating listing file for $(notdir $@)
	@$(MVASM) $(MVASMOPT) $< -o:$(patsubst %_shave.o,%.mobj,$@)
	@$(MVDUMP) -all $(patsubst %_shave.o,%.mobj,$@)
	@mkdir -p $(DirAppOutput)/lst/shave
	@mv $(patsubst %_shave.o,%.lst,$@) $(DirAppOutput)/lst/shave/$(patsubst %_shave.o,%.lst,$(notdir $@))
	@rm -rf $(patsubst %_shave.o,%.mobj,$@)
endif

%_shave.o : %.asm $(SHAVE_ASM_HEADERS) Makefile
	@echo Shave assembling $(notdir $@)
	$(ECHO) $(MVASM) $(MVASMOPT) -elf $< -o:$@
	@echo Generating listing file for $(notdir $@)
	@$(MVASM) $(MVASMOPT) $< -o:$(patsubst %_shave.o,%.mobj,$@)
	@$(MVDUMP) -all $(patsubst %_shave.o,%.mobj,$@)
	@mkdir -p $(DirAppOutput)/lst/shave
	@mv $(patsubst %_shave.o,%.lst,$@) $(DirAppOutput)/lst/shave/$(patsubst %_shave.o,%.lst,$(notdir $@))
	@rm -rf $(patsubst %_shave.o,%.mobj,$@)

%.asmgen : %.c $(SHAVE_C_HEADERS) Makefile
	@echo Shave C compiling $(notdir $@)
	$(ECHO) $(MVCC) $(MVCCOPT) $< -o $@

%.asmgen : %.cpp $(SHAVE_C_HEADERS) Makefile
	@echo Shave C++ compiling $(notdir $@)
	$(ECHO) $(MVCC) $(MVCCOPT) $(MVCCPPOPT) $< -o $@

%.i : %.c $(SHAVE_C_HEADERS) Makefile
	@echo Preprocessing shave C code $(notdir $@)
	$(ECHO) $(MVCC) $(MVCCOPT) -E $< -o $@

%.ipp : %.cpp $(SHAVE_C_HEADERS) Makefile
	@echo Preprocessing shave C code $(notdir $@)
	$(ECHO) $(MVCC) $(MVCCOPT) -E $< -o $@

#Move section prefixes for shave0 selected applications. This is the section describing 
#ABSOLUTE relocations
#That filter line there is just taking out the application name by removing the ".mvlib" extension
#and the leading words  
%.shv0lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv0. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))0_ $< $@
%.shv1lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv1. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))1_ $< $@
%.shv2lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv2. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))2_ $< $@
%.shv3lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv3. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))3_ $< $@
%.shv4lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv4. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))4_ $< $@
%.shv5lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv5. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))5_ $< $@
%.shv6lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv6. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))6_ $< $@
%.shv7lib : %.mvlib
	$(ECHO) $(OBJCOPY) --prefix-alloc-sections=.shv7. --prefix-symbols=$(lastword $(filter-out mvlib,$(subst /, ,$(subst ., ,$<))))7_ $< $@

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

#Targets for dynamically loadable objects
%.shvdlib : %.mvlib
	@echo Creating dynamically loadable library $@
	$(ECHO) $(LD) $(LDDYNOPT) $< -o $@

#this gives out all symbols too
%.shvdcomplete : %.mvlib
	@echo Creating windowed library for symbol extraction $@
	$(ECHO) $(LD) $(LDSYMOPT) $< -o $@

#this creates the required symbols file with only the symbols for the dynamic section
%_sym.o : %.shvdcomplete
	@echo Creating symbols file for the dynamic section $@
	$(ECHO) $(OBJCOPY) --prefix-symbols=$(lastword $(filter-out shvdcomplete,$(subst /, ,$(subst ., ,$<))))_ --extract-symbol $< $@

# General rule to make MVcmd from a elf
# [temp disable tools bug 17795]   $(ECHO) $(MVMOF) $(MVICFLAG) $(MVCMDOPT) $(^) -mvcmd:$(@)
$(DirAppOutput)/%.mvcmd : $(DirAppOutput)/%.elf
	@echo "Generating mvcmd target $(^)"
	@mkdir -p $(dir $@)
	$(ECHO) $(MVCONV) -elfInput $(MVCMDOPT) $(^) -mvcmd:$(@)

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
	$(ECHO) rm -rf $(LEON_OBJECTS_REQUIRED) $(PROJECTCLEAN) $(ALL_SHAVE_APPS) $(SHAVE_APP_LIBS) $(DirAppOutput)/$(APPNAME).map $(CommonMlibFile).mlib $(CommonMlibFile).mvlib $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(DirAppOutput)/lst

clean:
	@echo "Cleaning all built files from the MDK distribution and all project built files."
	$(ECHO) rm -rf $(LEON_DRIVER_OBJECTS_REQUIRED) $(SH_SWCOMMON_OBJS) $(SH_SWCOMMON_GENASMS) $(PROJECTCLEAN) $(LEON_OBJECTS_REQUIRED) $(SHAVE_APP_LIBS) $(ALL_SHAVE_APPS) $(DirAppOutput)/$(APPNAME).map $(DirAppOutput)/$(APPNAME).elf $(DirAppOutput)/$(APPNAME).mvcmd $(CommonMlibFile).mlib $(CommonMlibFile).mvlib $(ProjectShaveLib) $(DirAppOutput)/* 

.INTERMEDIARY: $(SHAVE_GENERATED_ASMS) $(PROJECTINTERM)
