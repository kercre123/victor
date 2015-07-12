# CLAD makefile by Pauley
CLAD_CPP=../tools/anki-util/tools/message-buffers/emitters/CPP_emitter.py
CLAD_C=../tools/anki-util/tools/message-buffers/emitters/C_emitter.py
CLAD_CSHARP=../tools/anki-util/tools/message-buffers/emitters/CSharp_emitter.py
INPUT_DIR=.
OUTPUT_DIR:=../generated/clad/
INPUT_FILES=$(shell find $(INPUT_DIR) -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_C=$(patsubst %.clad, %.c, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_H_AND_C=$(OUTPUT_H) $(OUTPUT_C)
OUTPUT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_H_AND_CPP)

REAL_OUTPUT_FILES=$(addprefix $(OUTPUT_DIR)/,$(OUTPUT_FILES))

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)
vpath %.cpp $(OUTPUT_DIR)
vpath %.cs $(OUTPUT_DIR)

.PHONY: show clean always

all: cpp

csharp: $(OUTPUT_CSHARP)

cpp: $(OUTPUT_H_AND_CPP)

c: $(OUTPUT_H_AND_C)

%.h %.cpp : %.clad always
	mkdir -p $(OUTPUT_DIR)/engine/$(dir $<)
	$(CLAD_CPP) -C $(INPUT_DIR) -o $(OUTPUT_DIR)/engine/ $(INPUT_DIR)/$*.clad

%.h %.c : %.clad always
	mkdir -p $(OUTPUT_DIR)/robot/$(dir $<)
	$(CLAD_C) -C $(INPUT_DIR) -o $(OUTPUT_DIR)/robot/ $(INPUT_DIR)/$*.clad

%.cs : %.clad always
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CSHARP) -C $(INPUT_DIR) -o $(OUTPUT_DIR)/unity/ $(INPUT_DIR)/$*.clad

show:
	echo "*** input_files ***"
	echo $(INPUT_FILES)
	echo "*** output_files ***"
	echo $(OUTPUT_FILES)

clean:
	rm $(REAL_OUTPUT_FILES)
