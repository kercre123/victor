# CLAD makefile by Pauley
CLAD_CPP=../lib/anki/cozmo-engine/tools/anki-util/tools/message-buffers/emitters/CPP_emitter.py
CLAD_C=../lib/anki/cozmo-engine/tools/anki-util/tools/message-buffers/emitters/C_emitter.py
CLAD_CSHARP=../lib/anki/cozmo-engine/tools/anki-util/tools/message-buffers/emitters/CSharp_emitter.py
CLAD_CPP_DECL=../lib/anki/cozmo-engine/clad/emitters/cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=../lib/anki/cozmo-engine/clad/emitters/cozmo_CPP_switch_emitter.py
INPUT_DIR=./src/
OUTPUT_DIR=../generated/clad/
OUTPUT_DIR_CSHARP=../unity/Cozmo/Assets/Scripts/Generated
INPUT_FILES=$(shell cd $(INPUT_DIR); find . -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_C=$(patsubst %.clad, %.c, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_H_AND_C=$(OUTPUT_H) $(OUTPUT_C)
OUTPUT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_H_AND_CPP)
INCLUDE_DIRS=../lib/anki/cozmo-engine/clad/src/

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.c $(OUTPUT_DIR)/robot/
vpath %.h $(OUTPUT_DIR)/game/
vpath %.cpp $(OUTPUT_DIR)/game/
vpath %.cs $(OUTPUT_DIR_CSHARP)

.PHONY: show clean

all: cpp csharp

csharp: $(OUTPUT_CSHARP)

cpp: $(OUTPUT_H_AND_CPP)

c: $(OUTPUT_H_AND_C)

%.h %.cpp : %.clad
	#mkdir -p $(OUTPUT_DIR)/game/$(dir $<)
	$(CLAD_CPP) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/game/ $*.clad
	$(CLAD_CPP_DECL) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/game/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/game/ $*.clad

%.h %.c : %.clad
	#mkdir -p $(OUTPUT_DIR)/robot/$(dir $<)
	$(CLAD_C) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad

%.cs : %.clad
	#mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CSHARP) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR_CSHARP)/ $*.clad

show:
	echo "*** input_files ***"
	echo $(INPUT_FILES)
	echo "*** output_files ***"
	echo $(OUTPUT_FILES)

clean:
	rm -rf $(OUTPUT_DIR) $(OUTPUT_DIR_CSHARP)
