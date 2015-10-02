# CLAD makefile for python

OUTPUT_DIR=./build/

PYTHON=python
PYTHONFLAGS=
EMITTER=../../Python_emitter.py
CLAD=$(PYTHON) $(PYTHONFLAGS) $(EMITTER)
PYTHON_TEST_SRC=pythontest.py

TREE_INPUT_DIR=../src/DirTreeTests
TREE_OUTPUT_DIR=$(OUTPUT_DIR)/tree

INPUT_FILES=$(subst $(TREE_INPUT_DIR)/, ,$(shell find $(TREE_INPUT_DIR) -type f -iname '*.clad'))
OUTPUT_PY=$(patsubst %.clad, $(TREE_OUTPUT_DIR)/%.py, $(INPUT_FILES))
OUTPUT_FILES=$(OUTPUT_PY)

ALIGNED_TREE_INPUT_DIR=../src/AlignedDirTreeTests
ALIGNED_TREE_OUTPUT_DIR=$(OUTPUT_DIR)/aligned-tree

ALIGNED_INPUT_FILES=$(subst $(ALIGNED_TREE_INPUT_DIR)/, ,$(shell find $(ALIGNED_TREE_INPUT_DIR) -type f -iname '*.clad'))
ALIGNED_OUTPUT_PY=$(patsubst %.clad, $(ALIGNED_TREE_OUTPUT_DIR)/%.py, $(ALIGNED_INPUT_FILES)) 
ALIGNED_OUTPUT_FILES=$(ALIGNED_OUTPUT_PY)

vpath %.clad $(TREE_INPUT_DIR)
vpath %.py $(TREE_OUTPUT_DIR)

vpath %.clad $(ALIGNED_TREE_INPUT_DIR)
vpath %.py $(ALIGNED_TREE_OUTPUT_DIR)

.PHONY: simple big show clean always

default: simple tree

all: simple tree big

simple:
	OUTPUT_DIR="$(OUTPUT_DIR)/simple" PYTHON="$(PYTHON)" PYTHONFLAGS="$(PYTHONFLAGS)" EMITTER="$(EMITTER)" ./simpletest.sh

big:
	@echo "No big python test yet"

tree: tree-aligned $(OUTPUT_FILES) test

$(TREE_OUTPUT_DIR)/%.py : %.clad always
	mkdir -p $(TREE_OUTPUT_DIR)/$(shell dirname $*.clad)
	$(CLAD) -C $(TREE_INPUT_DIR) -o $(TREE_OUTPUT_DIR) $*.clad

test: $(OUTPUT_FILES) always
	cp -f $(PYTHON_TEST_SRC) $(TREE_OUTPUT_DIR)
	$(PYTHON) $(PYTHONFLAGS) $(TREE_OUTPUT_DIR)/$(PYTHON_TEST_SRC)

show:
	@echo $(INPUT_FILES) $(ALIGNED_INPUT_FILES)

tree-aligned: $(ALIGNED_OUTPUT_FILES)

$(ALIGNED_TREE_OUTPUT_DIR)/%.py : %.clad always
	mkdir -p $(ALIGNED_TREE_OUTPUT_DIR)/$(shell dirname $*.clad)
	$(CLAD) -C $(ALIGNED_TREE_INPUT_DIR) -o $(ALIGNED_TREE_OUTPUT_DIR) $*.clad

clean:
	rm -rf $(OUTPUT_DIR)

