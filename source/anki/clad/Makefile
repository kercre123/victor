# CLAD makefile by Pauley
CLAD_EMITTER_DIR?=../../../tools/message-buffers/emitters
CLAD_CPP?=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CSHARP?=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_PYTHON?=$(CLAD_EMITTER_DIR)/Python_emitter.py
INPUT_DIR=.
OUTPUT_DIR:=./build
INPUT_FILES=$(shell find $(INPUT_DIR) -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_CSHARP=$(patsubst %.clad, %.cs, $(INPUT_FILES))
OUTPUT_PYTHON=$(patsubst %.clad, %.py, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_H_AND_CPP) $(OUTPUT_CSHARP)

REAL_OUTPUT_FILES=$(addprefix $(OUTPUT_DIR)/,$(OUTPUT_FILES))

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)
vpath %.cpp $(OUTPUT_DIR)
vpath %.cs $(OUTPUT_DIR)
vpath %.py $(OUTPUT_DIR)

.PHONY: show clean always

all: cpp

csharp: $(OUTPUT_CSHARP)

cpp: $(OUTPUT_H_AND_CPP)

python: $(OUTPUT_PYTHON)

%.h %.cpp : %.clad $(CLAD_CPP)
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CPP) -C $(INPUT_DIR) -o $(OUTPUT_DIR) --output-properties --output-json $(INPUT_DIR)/$*.clad

%.cs : %.clad $(CLAD_CSHARP)
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_CSHARP) -C $(INPUT_DIR) -o $(OUTPUT_DIR) $(INPUT_DIR)/$*.clad

%.py : %.clad $(CLAD_PYTHON)
	mkdir -p $(OUTPUT_DIR)/$(dir $<)
	$(CLAD_PYTHON) -C $(INPUT_DIR) -o $(OUTPUT_DIR) $(INPUT_DIR)/$*.clad

show:
	echo "*** input_files ***"
	echo $(INPUT_FILES)
	echo "*** output_files ***"
	echo $(OUTPUT_FILES)

clean:
	rm $(REAL_OUTPUT_FILES)
