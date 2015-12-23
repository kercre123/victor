# CLAD makefile by Pauley
# Customized for Robot by Daniel

CLAD_BASE_DIR=../../tools/anki-util/tools/message-buffers
CLAD_EMITTER_DIR=$(CLAD_BASE_DIR)/emitters
CLAD_LIB_DIR=$(CLAD_BASE_DIR)/clad
CLAD_DEPENDENCIES=$(CLAD_LIB_DIR)/*.py
CLAD_CPP=$(CLAD_EMITTER_DIR)/CPP_emitter.py
CLAD_CPPLITE=$(CLAD_EMITTER_DIR)/CPPLite_emitter.py
CLAD_CSHARP=$(CLAD_EMITTER_DIR)/CSharp_emitter.py
CLAD_PYTHON=$(CLAD_EMITTER_DIR)/Python_emitter.py
CLAD_CPP_DECL=./cozmo_CPP_declarations_emitter.py
CLAD_CPP_SWITCH=./cozmo_CPPLite_switch_emitter.py
CLAD_CPPLITE_SEND_HELPER=./cozmo_CPPLite_send_helper_emitter.py

INPUT_DIR=./src/
OUTPUT_DIR:=../generated/clad/
OUTPUT_DIR_PYTHON:=../generated/cladPython/
INPUT_FILES=$(shell cd $(INPUT_DIR); find . -type f -iname '*.clad')
OUTPUT_CPP=$(patsubst %.clad, %.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, %.h, $(INPUT_FILES))
OUTPUT_H_AND_CPP=$(OUTPUT_H) $(OUTPUT_CPP)
OUTPUT_PYTHON=$(patsubst %.clad, %.py, $(INPUT_FILES))
ENGINE_TO_ROBOT_UNION:=./clad/robotInterface/messageEngineToRobot
ROBOT_TO_ENGINE_UNION:=./clad/robotInterface/messageRobotToEngine

vpath %.clad $(INPUT_DIR)
vpath %.h $(OUTPUT_DIR)/robot/
vpath %.cpp $(OUTPUT_DIR)/robot/
vpath %.def $(OUTPUT_DIR)/robot/
vpath %.py $(OUTPUT_DIR_PYTHON)/robot/

.PHONY: show clean

all: cpp $(ENGINE_TO_ROBOT_UNION)_switch.def $(ENGINE_TO_ROBOT_UNION)_switch_group_anim.def $(ENGINE_TO_ROBOT_UNION)_declarations.def $(ROBOT_TO_ENGINE_UNION)_send_helper.h

cpp: $(OUTPUT_H_AND_CPP)

python: $(OUTPUT_PYTHON)

%_switch_group_anim.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch_group_anim.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch_group_anim.def; fi
	$(CLAD_CPP_SWITCH) --group=anim -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch_group_anim.def

%_switch.def : %.clad $(CLAD_CPP_SWITCH)
	if test -f $(OUTPUT_DIR)robot/$*_switch.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_switch.def; fi
	$(CLAD_CPP_SWITCH) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_switch.def

%_declarations.def : %.clad $(CLAD_CPP_DECL)
	if test -f $(OUTPUT_DIR)robot/$*_declarations.def; then chmod -f 777 $(OUTPUT_DIR)robot/$*_declarations.def; fi
	$(CLAD_CPP_DECL)   -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_declarations.def

%_send_helper.h : %.clad $(CLAD_CPPLITE_SEND_HELPER)
	if test -f $(OUTPUT_DIR)robot/$*_send_helper.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*_send_helper.h; fi
	$(CLAD_CPPLITE_SEND_HELPER) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*_send_helper.h

%.h %.cpp : %.clad $(CLAD_DEPENDENCIES) $(CLAD_CPPLITE)
	#mkdir -p $(OUTPUT_DIR)/robot/$(dir $<)
	if test -f $(OUTPUT_DIR)robot/$*.h; then chmod -f 777 $(OUTPUT_DIR)robot/$*.h; fi
	if test -f $(OUTPUT_DIR)robot/$*.cpp; then chmod -f 777 $(OUTPUT_DIR)robot/$*.cpp; fi
	$(CLAD_CPPLITE) --max-message-size 1400 -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR)/robot/ $*.clad
	chmod -f 555 $(OUTPUT_DIR)robot/$*.h
	chmod -f 555 $(OUTPUT_DIR)robot/$*.cpp

%.py : %.clad $(CLAD_DEPENDENCIES) $(CLAD_PYTHON)
	if test -f $(OUTPUT_DIR_PYTHON)robot/$*.py; then chmod -f 777 $(OUTPUT_DIR_PYTHON)robot/$*.py; fi
	$(CLAD_PYTHON) -C $(INPUT_DIR) -I $(INCLUDE_DIRS) -o $(OUTPUT_DIR_PYTHON)robot $*.clad
	chmod -f 555 $(OUTPUT_DIR_PYTHON)robot/$*.py

cpplilib: cpp
	@rm -f $(OUTPUT_DIR)/clad.cpp
	cd $(OUTPUT_DIR); find . -type f -iname '*.cpp' > cpps.list; awk '{ print "#include \"" $$0 "\"" }' <cpps.list >clad.cpp
	@rm $(OUTPUT_DIR)cpps.list

show:
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
