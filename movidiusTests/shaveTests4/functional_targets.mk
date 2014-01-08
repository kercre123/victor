#functional_targets.mk - Features functional targets to be used with the new build flow
#
# Created on: Aug 19, 2013
#     Author: Cristian-Gavril Olar

# Default IP address for debugger connection, can be overridden		     
srvIP               ?= 127.0.0.1   
#srvPort             ?= 4001
srvPort             ?= 

# This target is used by the build regression to detect which projects are MDK top-level projects
isMdkProject:
	@echo $(MDK_PROJ_STRING)

run: $(DirAppOutput)/$(APPNAME).elf
	@# This target is used to simply run your application.
	@# It performs load app; runw; exit 
	@cp $(SourceDebugScript) $(debugSCR)
	$(MVDBG) -b:$(debugSCR) $(MVDBGOPT) -srvIP:$(srvIP) -D:elf:$(DirAppOutput)/$(APPNAME).elf -D:default_target:$(MV_DEFAULT_START_PROC_ID)  -D:run_opt:runw -D:exit_opt:exit 

debug: $(DirAppOutput)/$(APPNAME).elf
	@# This target is used to load and run your application, but interactive debug is still possible during exection
	@# It performs load app; run;
	@cp $(SourceDebugScript) $(debugSCR)
	$(MVDBG) -b:$(debugSCR) $(MVDBGOPT) -srvIP:$(srvIP) -D:elf:$(DirAppOutput)/$(APPNAME).elf -D:default_target:$(MV_DEFAULT_START_PROC_ID)  -D:run_opt:run  -D:exit_opt:" "

load: $(DirAppOutput)/$(APPNAME).elf
	@# This target is used to simply start debugger and load your application ready for interactive debug
	@# It performs load app; 
	@cp $(SourceDebugScript) $(debugSCR)
	$(MVDBG) -b:$(debugSCR) $(MVDBGOPT) -srvIP:$(srvIP) -D:elf:$(DirAppOutput)/$(APPNAME).elf -D:default_target:$(MV_DEFAULT_START_PROC_ID)  -D:run_opt:" "  -D:exit_opt:"echo type 'run' to start your application"

debugi: 
	@# Simply launch moviDebug interactively
	$(MVDBG) $(MVDBGOPT) -srvIP:$(srvIP) -D:elf:$(DirAppOutput)/$(APPNAME).elf
			    
start_server:
	while true; do $(MVSVR); done

start_simulator:
	while true; do $(MVSIM) $(MVSIMOPT) -q; done

start_simulator_full:
	while true; do $(MVSIM) $(MVSIMOPT) -darw; done
	

sim: $(DirAppOutput)/$(APPNAME).elf
	$(MVSIM) $(MVSIMOPT) $(SIM_OPT) -l:$(MV_DEFAULT_START_PROC_ID):$(DirAppOutput)/$(APPNAME).elf $(MVSIMOUTOPT) | tee $(DirOutputLst)/leonos_movisim.lst
	-$(ECHO)cat $(DirOutputLst)/leonos_movisim.lst  | grep "PC=0x" | sed 's/^PC=0x//g' > $(DirOutputLst)/leonos_movisim.dasm
	-$(ECHO)cat $(DirOutputLst)/leonos_movisim.lst  | grep -w "UART:\|DEBUG:" > $(DirOutputLst)/leonos_movisim_uart.txt

# Programs the target application into SPI Memory on MV0153
flash: MVCMDSIZE = $(shell du -b $(MvCmdfile))
flash: $(MvCmdfile)	
	$(ECHO)cat $(DefaultFlashScript) | \
		sed 's!XX_FLASHER_ELF_XX!$(MV_COMMON_BASE)/utils/jtag_flasher/flasher.elf!' | \
		sed 's!XX_TARGET_MVCMD_SIZE_XX!$(word 1, $(MVCMDSIZE))!' | \
		sed 's!XX_TARGET_MVCMD_XX!$(MvCmdfile)!' > $(flashSCR)
	$(ECHO)$(MVDBG) -b:$(flashSCR) $(MVDBGOPT) -srvIP:$(srvIP)
	$(ECHO) if diff $(MvCmdfile) $(MvCmdfile).readback; then \
		echo "Reading back from DDR unchanged [OK]"; \
		rm $(MvCmdfile).readback; \
	else \
		echo "Reading back from DDR was different! [BAD]"; \
		exit 1; \
	fi
	
flash_erase:
	$(ECHO)cat $(DefaultEraseScript) | \
		sed 's!XX_FLASHER_ELF_XX!$(MV_COMMON_BASE)/utils/jtag_flasher/flasher.elf!' | \
		sed 's!XX_TARGET_MVCMD_SIZE_XX!0!' > $(eraseSCR)
	$(ECHO)$(MVDBG) -b:$(eraseSCR) $(MVDBGOPT) -srvIP:$(srvIP)

report:
	@echo "Generating memory view..."
	@mkdir -p $(DirOutputReport)
	@python $(MV_COMMON_BASE)/utils/memviz.py -i $(DirAppOutput)/$(APPNAME).map \
	 -o $(DirOutputReport)/memviz.html -c ../../$(MV_COMMON_BASE)/utils/memviz_style.css
help:
	@echo "'make help'                      : show this message"
	@echo "'make all'                       : build everything"
	@echo "'make clean'                     : Clean all built files from all over the"
	@echo "                                   MDK distribution (including build drivers "
	@echo "                                   files)"
	@echo "'make run'                       : build everything and run it on a target via"
	@echo "                                   moviDebug (non-interactive)"
	@echo "'make debug'                     : build application and interactively debug it"
	@echo "                                   on a target via moviDebug. Application is loaded"
	@echo "                                   and execution is started automatically"
	@echo "'make load'                      : build application and load target elf into debugger"
	@echo "                                   but don't start execution. (allows setting "
	@echo "                                   breakpoints in advance)"
	@echo "'make debugi'                    : Simply launch debugger for interactive session"
	@echo "                                   (no commands executed)"
	@echo "'make start_server'              : starts moviDebug server"
	@echo "'make start_simulator'           : starts moviSim in quiet mode"
	@echo "'make start_simulator_full       : starts moviSim in verbose full mode"
	@echo "'make report'                    : generates a visual report of memory utilization."
	@echo "                                   Needs application to be build previously"
	@echo "'make show_tools'                : display tool paths"
	@echo "'make flash'                     : Program SPI flash of MV0153 with "
	@echo "                                   your current mvcmd"
	@echo "'make flash_erase'               : Erase SPI flash of MV0153"
	@echo 
	@echo "Optional Parameters:"
	@echo "srvIP=192.168.1.100              : Connect to moviDebugserver on a different PC"
	@echo "VERBOSE=yes                      : Echo full build commands during make"
	@echo "DISABLE_PRINTF=yes               : Turns off printf, useful when generating Boot"
	@echo "                                   image"
	@echo "DEBUG=yes                        : Enables -DDEBUG in compiler (causes asserts"
	@echo "                                   to print messages)"
	@echo "MvCmdfile=path/to/file.mvcmd     : given to \"make flash\" to write a "
	@echo "                                   different .mvcmd file"
	@echo 

show_tools:
	@echo "SHAVE Toolchain..."
	@echo "   MVLNK   :  $(MVLNK)"
	@echo "   MVASM   :  $(MVASM)"
	@echo "   MVDUMP  :  $(MVDUMP)"
	@echo "   MVSIM   :  $(MVSIM)"
	@echo "   MVDBG   :  $(MVDBG)"
	@echo "   MVDBGTCL:  $(MVDBGTCL)"
	@echo "   MVCONV  :  $(MVCONV)"
	@echo "   MVCC    :  $(MVCC)" 
	@echo "   MVSVR   :  $(MVSVR)"
	@echo ""
	@echo "Sparc (Leon) Toolchain..."
	@echo "   CC      :  $(CC)"
	@echo "   LD      :  $(LD)"
	@echo "   OBJCOPY :  $(OBJCOPY)"
	@echo "   AS      :  $(AS)"
	@echo "   AR      :  $(AR)"
	@echo "   STRIP   :  $(STRIP)"
	@echo "   OBJDUMP :  $(OBJDUMP)"


