# CLAD makefile by Pauley

CLAD_BASE_DIR=../tools/anki-util/tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_C=$(CLAD_EMITTER_DIR)/C_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_CPP_DECL=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPP_switch_emitter.py
INPUT_DIR=./src/ # INPUT_DIR gets overridden by GYP to build robot interface stuff
ROBOT_CLAD_SRC=../robot/clad/src/
ROBOT_SHARED_CLAD_SRC=../robot/cladShared/src/
OUTPUT_DIR:=../generated/clad/
OUTPUT_DIR_CSHARP=../generated/clad/unity/
INPUT_FILES=$(shell cd $(INPUT_DIR); find . -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_C=$(patsubst %.clad, %.c, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_H_AND_C=$(OUTPUT_H) $(OUTPUT_C)
OUTPUT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_H_AND_CPP)

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.c $(OUTPUT_DIR)/robot/
vpath %.h $(OUTPUT_DIR)/engine/
vpath %.cpp $(OUTPUT_DIR)/engine/
vpath %.cs $(OUTPUT_DIR_CSHARP)

.PHONY: show clean

all: cpp

csharp: $(OUTPUT_CSHARP)

cpp: $(OUTPUT_H_AND_CPP)

c: $(OUTPUT_H_AND_C)

%.h %.cpp : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CPP) $(CLAD_CPP_DECL) $(CLAD_CPP_SWITCH)
	#mkdir -p $(OUTPUT_DIR)/engine/$(dir $<)
	if test -f $(OUTPUT_DIR)engine/$*.h; then chmod -f 777 $(OUTPUT_DIR)engine/$*.h; fi
	if test -f $(OUTPUT_DIR)engine/$*.cpp; then chmod -f 777 $(OUTPUT_DIR)engine/$*.cpp; fi
	$(CLAD_CPP) --output-union-helper-constructors -C $(INPUT_DIR) -I $(ROBOT_CLAD_SRC) -I $(ROBOT_SHARED_CLAD_SRC) -o $(OUTPUT_DIR)/engine/ $*.clad
	$(CLAD_CPP_DECL) -C $(INPUT_DIR) -I $(INCLUDE_DIRS)  -I $(ROBOT_CLAD_SRC) -I $(ROBOT_SHARED_CLAD_SRC) -o $(OUTPUT_DIR)/engine/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -I $(ROBOT_CLAD_SRC) -I $(ROBOT_SHARED_CLAD_SRC) -o $(OUTPUT_DIR)/engine/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)engine/$*.h
	chmod -f 555 $(OUTPUT_DIR)engine/$*.cpp

%.h %.c : %.clad $(CLAD_DEPENDENCIES) $(CLAD_C)
	#mkdir -p $(OUTPUT_DIR)/robot/$(dir $<)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	if test -f $(OUTPUT_DIR)robot/$*.c; then chmod -f 777 $(OUTPUT_DIR)robot/$*.c; fi
	$(CLAD_C) -C $(INPUT_DIR) -I $(ROBOT_CLAD_SRC) -I $(ROBOT_SHARED_CLAD_SRC) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h
	chmod -f 555 $(OUTPUT_DIR)robot/$*.c

%.cs : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CSHARP)
	#mkdir -p $(OUTPUT_DIR)/$(dir $<)
	if test -f $(OUTPUT_DIR_CSHARP)/$*.cs; then chmod -f 777 $(OUTPUT_DIR_CSHARP)/$*.cs; fi
	$(CLAD_CSHARP) -C $(INPUT_DIR) -I $(ROBOT_CLAD_SRC) -I $(ROBOT_SHARED_CLAD_SRC) -o $(OUTPUT_DIR_CSHARP)/ $*.clad
	chmod -f 555 $(OUTPUT_DIR_CSHARP)/$*.cs

show:
	echo "*** input_files ***"
	echo $(INPUT_FILES)
	echo "*** output_files ***"
	echo $(OUTPUT_FILES)

clean:
	if test -d $(OUTPUT_DIR); then chmod -Rf 777 $(OUTPUT_DIR); fi
	if test -d $(OUTPUT_DIR_CSHARP); then chmod -Rf 777 $(OUTPUT_DIR_CSHARP); fi
	rm -rf $(OUTPUT_DIR) $(OUTPUT_DIR_CSHARP)
