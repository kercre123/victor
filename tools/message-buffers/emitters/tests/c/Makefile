# CLAD makefile for C

OUTPUT_DIR=./build

PYTHON=python
PYTHONFLAGS=
EMITTER=../../C_emitter.py
CLAD=$(PYTHON) $(PYTHONFLAGS) $(EMITTER)
C_TEST_SRC=ctest.c
CC=clang
CFLAGS:=-Wall -Wextra -Os -g

ALIGNED_TREE_INPUT_DIR=../src/AlignedDirTreeTests
ALIGNED_TREE_OUTPUT_DIR=$(OUTPUT_DIR)/aligned-tree

ALIGNED_INPUT_FILES=$(subst $(ALIGNED_TREE_INPUT_DIR)/, ,$(shell find $(ALIGNED_TREE_INPUT_DIR) -type f -iname '*.clad'))
ALIGNED_OUTPUT_H=$(patsubst %.clad, $(ALIGNED_TREE_OUTPUT_DIR)/%.h, $(ALIGNED_INPUT_FILES))
ALIGNED_OUTPUT_FILES=$(ALIGNED_OUTPUT_H)
ALIGNED_TEST_BIN=$(ALIGNED_TREE_OUTPUT_DIR)/testAlignedC

vpath %.clad $(ALIGNED_TREE_INPUT_DIR)
vpath %.h $(ALIGNED_TREE_OUTPUT_DIR)

.PHONY: simple big show clean always

default: simple tree 

all: simple tree big

simple:
	OUTPUT_DIR="$(OUTPUT_DIR)/simple" PYTHON="$(PYTHON)" PYTHONFLAGS="$(PYTHONFLAGS)" EMITTER="$(EMITTER)" ./simpletest.sh

big:
	OUTPUT_DIR="$(OUTPUT_DIR)/big" PYTHON="$(PYTHON)" PYTHONFLAGS="$(PYTHONFLAGS)" EMITTER="$(EMITTER)" ./bigtest.sh

tree: $(ALIGNED_OUTPUT_FILES) test

$(ALIGNED_TREE_OUTPUT_DIR)/%.h : %.clad always
	mkdir -p $(ALIGNED_TREE_OUTPUT_DIR)/$(shell dirname $*.clad)
	$(CLAD) -C $(ALIGNED_TREE_INPUT_DIR) -o $(ALIGNED_TREE_OUTPUT_DIR) $*.clad

$(ALIGNED_TEST_BIN): $(C_TEST_SRC)
	$(CC) $(CFLAGS) -I$(ALIGNED_TREE_OUTPUT_DIR) -o $@ $^

test: $(ALIGNED_TEST_BIN) always
	$(ALIGNED_TEST_BIN)

show:
	@echo $(ALIGNED_INPUT_FILES)

clean:
	rm -rf $(OUTPUT_DIR)

