# CLAD makefile by Pauley
# Customized for Robot by Daniel
# Customized for Robot2 by Adam

CLAD_BASE_DIR=../../tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CPPLITE=$(CLAD_EMITTER_DIR)/CPPLite_emitter.py
CLAD_PURE_C=$(CLAD_EMITTER_DIR)/C_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_PYTHON=$(CLAD_EMITTER_DIR)/Python_emitter.py
CLAD_CPP_DECL=./cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=./cozmo_CPPLite_switch_emitter.py
CLAD_CPPLITE_SEND_HELPER=./cozmo_CPPLite_send_helper_emitter.py
CLAD_HASH=$(CLAD_EMITTER_DIR)/ASTHash_emitter.py

INPUT_DIR=./src/
OUTPUT_DIR:=../generated/clad/
OUTPUT_DIR_PYTHON:=../generated/cladPython/
SPINE_FILES=$(shell cd $(INPUT_DIR); /usr/bin/find . -type f -iname 'spine*.clad')
INPUT_FILES=$(shell cd $(INPUT_DIR); /usr/bin/find . -type f ! -iname 'spine*.clad' -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES) $(SPINE_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_PYTHON=$(patsubst %.clad, %.py, $(INPUT_FILES))

PYTHON=python3

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.cpp $(OUTPUT_DIR)/robot/
vpath %.def $(OUTPUT_DIR)/robot/
vpath %.py $(OUTPUT_DIR_PYTHON)/robot/

.PHONY: show clean

all: cpp python

cpp: $(OUTPUT_H_AND_CPP) 

python: $(OUTPUT_PYTHON) 

%_switch_group_anim.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_group_anim.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_group_anim.def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) --group=anim -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_group_anim.def

%_switch.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch.def; fi
	$(PYTHON) $(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch.def

%_declarations.def : %.clad $(CLAD_CPP_DECL)
	if test -f $(OUTPUT_DIR)robot/$*_declarations.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_declarations.def; fi
	$(PYTHON) $(CLAD_CPP_DECL)   -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_declarations.def

%.h :  $(SPINE_FILES) $(CLAD_DEPENDENCIES) $(CLAD_PURE_C)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	$(PYTHON) $(CLAD_PURE_C)  -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h

%.h %.cpp : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CPPLITE)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	if test -f $(OUTPUT_DIR)robot/$*.cpp; then chmod -f 777 $(OUTPUT_DIR)robot/$*.cpp; fi
	$(PYTHON) $(CLAD_CPPLITE) --max-message-size 1400 -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h
	chmod -f 555 $(OUTPUT_DIR)robot/$*.cpp

%_hash.h : %.clad $(CLAD_DEPENDENCIES) $(CLAD_HASH) $(shell /usr/bin/find $(INPUT_DIR) -type f)
	if test -f $(OUTPUT_DIR)robot/$@; then chmod -f 777 $(OUTPUT_DIR)robot/$@; fi
	$(PYTHON) $(CLAD_HASH) -I $(INPUT_DIR) -o $(OUTPUT_DIR)robot/$@ $<
	chmod -f 555 $(OUTPUT_DIR)robot/$@

%_hash.py: %.clad $(CLAD_DEPENDENCIES) $(CLAD_HASH) $(shell /usr/bin/find $(INPUT_DIR) -type f)
	if test -f $(OUTPUT_DIR_PYTHON)robot/$@; then chmod -f 777 $(OUTPUT_DIR_PYTHON)robot/$@; fi
	$(PYTHON) $(CLAD_HASH) -I $(INPUT_DIR) -o $(OUTPUT_DIR_PYTHON)robot/$@ $<
	chmod -f 555 $(OUTPUT_DIR_PYTHON)robot/$@

%.py : %.clad $(CLAD_DEPENDENCIES) $(CLAD_PYTHON)
	if test -f $(OUTPUT_DIR_PYTHON)robot/$*.py; then chmod -f 777 $(OUTPUT_DIR_PYTHON)robot/$*.py; fi
	$(PYTHON) $(CLAD_PYTHON) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR_PYTHON)robot $*.clad
	chmod -f 555 $(OUTPUT_DIR_PYTHON)robot/$*.py

cpplilib: cpp
	@rm -f $(OUTPUT_DIR)/clad.cpp
	cd $(OUTPUT_DIR); /usr/bin/find . -type f -iname '*.cpp' > cpps.list; awk '{ print "#include \"" $$0 "\"" }' <cpps.list >clad.cpp
	@rm $(OUTPUT_DIR)cpps.list


show:
	echo "*** SPINE files ***"
	echo $(SPINE_FILES)
	echo "*** input files ***"
	echo $(INPUT_FILES)
	echo "*** output C++ files ***"
	echo $(OUTPUT_H_AND_CPP)
	echo "*** output python files ***"
	echo $(OUTPUT_PYTHON)

clean:
	if test -d $(OUTPUT_DIR); then chmod -Rf 777 $(OUTPUT_DIR); fi
	if test -d $(OUTPUT_DIR_PYTHON); then chmod -Rf 777 $(OUTPUT_DIR_PYTHON); fi
	rm -rf $(OUTPUT_DIR) $(OUTPUT_DIR_PYTHON)
