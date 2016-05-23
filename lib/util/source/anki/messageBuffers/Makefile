# CLAD makefile by Pauley
CLAD_CPP=../../../tools/message-buffers/emitters/CPP_emitter.py
CLAD_CSHARP=../../../tools/message-buffers/emitters/CSharp_emitter.py
INPUT_DIR=.
OUTPUT_DIR:=./build
INPUT_FILES=$(shell find $(INPUT_DIR) -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_H_AND_CPP) $(OUTPUT_CSHARP)

REAL_OUTPUT_FILES=$(addprefix $(OUTPUT_DIR)/,$(OUTPUT_FILES))

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)
vpath %.cpp $(OUTPUT_DIR)
vpath %.cs $(OUTPUT_DIR)

.PHONY: show clean always

all: cpp

csharp: $(OUTPUT_CSHARP)

cpp: $(OUTPUT_H_AND_CPP)

%.h %.cpp : %.clad always
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CPP) -C $(INPUT_DIR) -o $(OUTPUT_DIR) $(INPUT_DIR)/$*.clad

%.cs : %.clad always
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CSHARP) -C $(INPUT_DIR) -o $(OUTPUT_DIR) $(INPUT_DIR)/$*.clad

show:
	echo "*** input_files ***"
	echo $(INPUT_FILES)
	echo "*** output_files ***"
	echo $(OUTPUT_FILES)

clean:
	rm $(REAL_OUTPUT_FILES)
