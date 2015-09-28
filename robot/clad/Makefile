# CLAD makefile by Pauley
# Customized for Robot by Daniel

CLAD_BASE_DIR=../../tools/anki-util/tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CPPLITE=$(CLAD_EMITTER_DIR)/CPPLite_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_CPP_DECL=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPPLite_switch_emitter.py
CLAD_CPPLITE_SEND_HELPER=$(CLAD_EMITTER_DIR)/specialized/cozmo_CPPLite_send_helper_emitter.py

INPUT_DIR=./src/
OUTPUT_DIR:=../generated/clad/
INPUT_FILES=$(shell cd $(INPUT_DIR); find . -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.cpp $(OUTPUT_DIR)/robot/

.PHONY: show clean

all: cpp

cpp: $(OUTPUT_H_AND_CPP)

%.h %.cpp : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CPPLITE) $(CLAD_CPP_DECL) $(CLAD_CPP_SWITCH)
	#mkdir -p $(OUTPUT_DIR)/robot/$(dir $<)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	if test -f $(OUTPUT_DIR)robot/$*.cpp; then chmod -f 777 $(OUTPUT_DIR)robot/$*.cpp; fi
	$(CLAD_CPPLITE) --max-message-size 1400 -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	$(CLAD_CPP_DECL)   -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	$(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	$(CLAD_CPPLITE_SEND_HELPER) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h
	chmod -f 555 $(OUTPUT_DIR)robot/$*.cpp

show:
	echo "*** input files ***"
	echo $(INPUT_FILES)
	echo "*** output files ***"
	echo $(OUTPUT_H_AND_CPP)

clean:
	if test -d $(OUTPUT_DIR); then chmod -Rf 777 $(OUTPUT_DIR); fi
	rm -rf $(OUTPUT_DIR)
