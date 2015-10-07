# CLAD makefile by Pauley

CLAD_BASE_DIR=../tools/anki-util/tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_CPP_DECL=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPP_switch_emitter.py

OUTPUT_DIR:=../generated/clad/
OUTPUT_DIR_CSHARP=../generated/cladCsharp/
OUTPUT_DIR_ENGINE:=$(OUTPUT_DIR)/engine/

INPUT_ENGINE_DIR=./src/
INPUT_ENGINE_FILES=$(shell cd $(INPUT_ENGINE_DIR); find . -type f -iname '*.clad')
OUTPUT_ENGINE_CPP=$(patsubst %.clad, %.cpp, $(INPUT_ENGINE_FILES))
OUTPUT_ENGINE_H=$(patsubst %.clad, %.h, $(INPUT_ENGINE_FILES))
OUTPUT_ENGINE_H_AND_CPP=$(OUTPUT_ENGINE_H) $(OUTPUT_ENGINE_CPP)
OUTPUT_ENGINE_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_ENGINE_FILES))

INPUT_VIZ_DIR=./vizSrc/
INPUT_VIZ_FILES=$(shell cd $(INPUT_VIZ_DIR); find . -type f -iname '*.clad')
OUTPUT_VIZ_CPP=$(patsubst %.clad, %.cpp, $(INPUT_VIZ_FILES))
OUTPUT_VIZ_H=$(patsubst %.clad, %.h, $(INPUT_VIZ_FILES))
OUTPUT_VIZ_H_AND_CPP=$(OUTPUT_VIZ_H) $(OUTPUT_VIZ_CPP)
OUTPUT_VIZ_LST=../generated/clad/viz.lst

INPUT_ROBOT_DIR=../robot/clad/src/
INPUT_ROBOT_NOT_SHARED_FILES=$(shell cd $(INPUT_ROBOT_DIR); grep -irL ROBOT_CLAD_SHARED *)
INPUT_ROBOT_SHARED_FILES=$(shell cd $(INPUT_ROBOT_DIR); grep -irl ROBOT_CLAD_SHARED *)
INPUT_ROBOT_CPP_FILES=$(INPUT_ROBOT_SHARED_FILES) $(INPUT_ROBOT_NOT_SHARED_FILES)
OUTPUT_ROBOT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_ROBOT_CPP_FILES))
OUTPUT_ROBOT_H=$(patsubst %.clad, %.h, $(INPUT_ROBOT_CPP_FILES))
OUTPUT_ROBOT_H_AND_CPP=$(OUTPUT_ROBOT_H) $(OUTPUT_ROBOT_CPP)
OUTPUT_ROBOT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_ROBOT_SHARED_FILES))


vpath %.clad $(INPUT_ENGINE_DIR):$(INPUT_VIZ_DIR):$(INPUT_ROBOT_DIR)
vpath %.h $(OUTPUT_DIR_ENGINE)/
vpath %.cpp $(OUTPUT_DIR_ENGINE)/
vpath %.cs $(OUTPUT_DIR_CSHARP)/

.PHONY: show clean $(OUTPUT_VIZ_LST)

all: cpp $(OUTPUT_VIZ_LST)

cpp: engineCpp robotCpp vizCpp
robotCpp: $(OUTPUT_ROBOT_CPP)
engineCpp: $(OUTPUT_ENGINE_H_AND_CPP)
vizCpp: $(OUTPUT_VIZ_H_AND_CPP) 
csharp: engineCsharp robotCsharp
robotCsharp: $(OUTPUT_ROBOT_CSHARP)
engineCsharp: $(OUTPUT_ENGINE_CSHARP)


$(OUTPUT_VIZ_H_AND_CPP): $(INPUT_VIZ_FILES) $(CLAD_DEPENDENCIES) $(CLAD_CPP) $(CLAD_CPP_DECL) $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR_ENGINE)/$*.h; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.h; fi
	if test -f $(OUTPUT_DIR_ENGINE)/$*.cpp; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.cpp; fi
	$(CLAD_CPP) --output-union-helper-constructors -C $(INPUT_VIZ_DIR) -I $(INPUT_ROBOT_DIR) $(INPUT_ENGINE_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_DECL) -C $(INPUT_VIZ_DIR) -I $(INPUT_ROBOT_DIR) $(INPUT_ENGINE_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_VIZ_DIR) -I $(INPUT_ROBOT_DIR) $(INPUT_ENGINE_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.h
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.cpp


$(OUTPUT_ROBOT_H_AND_CPP): $(INPUT_ROBOT_CPP_FILES) $(CLAD_DEPENDENCIES) $(CLAD_CPP) $(CLAD_CPP_DECL) $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR_ENGINE)/$*.h; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.h; fi
	if test -f $(OUTPUT_DIR_ENGINE)/$*.cpp; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.cpp; fi
	$(CLAD_CPP) --output-union-helper-constructors -C $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_DECL) -C $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.h
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.cpp


$(OUTPUT_ENGINE_H_AND_CPP): $(INPUT_ENGINE_FILES) $(CLAD_DEPENDENCIES) $(CLAD_CPP) $(CLAD_CPP_DECL) $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR_ENGINE)/$*.h; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.h; fi
	if test -f $(OUTPUT_DIR_ENGINE)/$*.cpp; then chmod -f 777 $(OUTPUT_DIR_ENGINE)/$*.cpp; fi
	$(CLAD_CPP) --output-union-helper-constructors -C $(INPUT_ENGINE_DIR) -I $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_DECL) -C $(INPUT_ENGINE_DIR) -I $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_ENGINE_DIR) -I $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_ENGINE)/ $*.clad
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.h
	chmod -f 555 $(OUTPUT_DIR_ENGINE)/$*.cpp


# in this target stem does not work. probably issue with patsubst,
# instead of $*.clad use $(@:.cs=.clad)
$(OUTPUT_ROBOT_CSHARP): $(INPUT_ROBOT_SHARED_FILES) $(CLAD_DEPENDENCIES) $(CLAD_CSHARP)
	if test -f $(OUTPUT_DIR_CSHARP)/$@; then chmod -f 777 $(OUTPUT_DIR_CSHARP)/$@; fi
	$(CLAD_CSHARP) -C $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_CSHARP)/ $(@:.cs=.clad)
	chmod -f 555 $(OUTPUT_DIR_CSHARP)/$@


# in this target stem does not work. probably issue with patsubst,
# instead of $*.clad use $(@:.cs=.clad)
$(OUTPUT_ENGINE_CSHARP): $(INPUT_ENGINE_FILES) $(CLAD_DEPENDENCIES) $(CLAD_CSHARP)
	if test -f $(OUTPUT_DIR_CSHARP)/$@; then chmod -f 777 $(OUTPUT_DIR_CSHARP)/$@; fi
	$(CLAD_CSHARP) -C $(INPUT_ENGINE_DIR) -I $(INPUT_ROBOT_DIR) -o $(OUTPUT_DIR_CSHARP)/ $(@:.cs=.clad)
	chmod -f 555 $(OUTPUT_DIR_CSHARP)/$@


# clad specific paths in here..
$(OUTPUT_VIZ_LST): 
	#$(INPUT_VIZ_FILES)
	cat /dev/null > $(OUTPUT_VIZ_LST)
	for f in $(OUTPUT_VIZ_H_AND_CPP); do echo ../$(OUTPUT_DIR_ENGINE)/$$f >> $(OUTPUT_VIZ_LST); done
	

show:
	echo "*** input files ***"
	echo $(INPUT_ENGINE_FILES)
	echo "*** output files engine cpp ***"
	echo $(OUTPUT_ENGINE_H_AND_CPP)
	echo "*** input files robot ***"
	echo $(INPUT_ROBOT_SHARED_FILES)
	echo "*** output files robot cpp ***"
	echo $(OUTPUT_ROBOT_H_AND_CPP)
	

clean:
	if test -d $(OUTPUT_DIR_ENGINE); then chmod -Rf 777 $(OUTPUT_DIR_ENGINE); fi
	if test -d $(OUTPUT_DIR_CSHARP); then chmod -Rf 777 $(OUTPUT_DIR_CSHARP); fi
	rm -rf $(OUTPUT_DIR_ENGINE) $(OUTPUT_DIR_CSHARP) $(OUTPUT_VIZ_LST)
