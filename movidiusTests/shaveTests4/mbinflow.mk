#mbinflow.mk - simple mbinflow.mk rules collection to be used with the Movidius build flow
#
# Created on: Aug 27, 2013
#     Author: Cristian-Gavril Olar

%.mobj : %.asmgen $(SHAVE_ASM_HEADERS) Makefile
	@echo Shave assembling $@
	$(ECHO) $(MVASM) $(MVASMOPT) $< -o:$@

%.mobj : %.asm $(SHAVE_ASM_HEADERS) Makefile
	@echo Shave assembling $@
	$(ECHO) $(MVASM) $(MVASMOPT) $< -o:$@

%_raw.o : %.mbin
	$(ECHO) $(OBJCOPY)  -I binary --reverse-bytes=4 --rename-section .data=.ddr_direct.data \
	--redefine-sym  _binary_$(subst /,_,$(subst .,_,$<))_start=$(lastword $(filter-out mbin,$(subst /, ,$(subst ., ,$<))))Mbin \
	-O elf32-sparc -B sparc $< $@