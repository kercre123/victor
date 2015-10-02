# CLAD makefile for C++

OUTPUT_DIR=./build

PYTHON=python
PYTHONFLAGS=
EMITTER=../../CPP_emitter.py
CLAD=$(PYTHON) $(PYTHONFLAGS) $(EMITTER)
CPP_TEST_SRC=cpptest.cpp
CXX=clang++
CXXFLAGS:=-Wall -Wextra -std=c++11 -stdlib=libc++ -Os -g

TREE_INPUT_DIR=../src/DirTreeTests
TREE_OUTPUT_DIR=$(OUTPUT_DIR)/tree
TREE_SUPPORT_DIR=../../../support/cpp

CLAD_SUPPORT_SRC=$(TREE_SUPPORT_DIR)/source/SafeMessageBuffer.cpp
INPUT_FILES=$(subst $(TREE_INPUT_DIR)/, ,$(shell find $(TREE_INPUT_DIR) -type f -iname '*.clad'))
OUTPUT_CPP=$(patsubst %.clad, $(TREE_OUTPUT_DIR)/%.cpp, $(INPUT_FILES))
OUTPUT_H=$(patsubst %.clad, $(TREE_OUTPUT_DIR)/%.h, $(INPUT_FILES)) 
OUTPUT_FILES=$(OUTPUT_H) $(OUTPUT_CPP)
TEST_BIN=$(TREE_OUTPUT_DIR)/testCPP

ALIGNED_TREE_INPUT_DIR=../src/AlignedDirTreeTests
ALIGNED_TREE_OUTPUT_DIR=$(OUTPUT_DIR)/aligned-tree

ALIGNED_INPUT_FILES=$(subst $(ALIGNED_TREE_INPUT_DIR)/, ,$(shell find $(ALIGNED_TREE_INPUT_DIR) -type f -iname '*.clad'))
ALIGNED_OUTPUT_CPP=$(patsubst %.clad, $(ALIGNED_TREE_OUTPUT_DIR)/%.cpp, $(ALIGNED_INPUT_FILES)) 
ALIGNED_OUTPUT_H=$(patsubst %.clad, $(ALIGNED_TREE_OUTPUT_DIR)/%.h, $(ALIGNED_INPUT_FILES)) 
ALIGNED_OUTPUT_FILES=$(ALIGNED_OUTPUT_H) $(ALIGNED_OUTPUT_CPP)

vpath %.clad $(TREE_INPUT_DIR)
vpath %.h $(TREE_OUTPUT_DIR)
vpath %.cpp $(TREE_OUTPUT_DIR)

vpath %.clad $(ALIGNED_TREE_INPUT_DIR)
vpath %.h $(ALIGNED_TREE_OUTPUT_DIR)
vpath %.cpp $(ALIGNED_TREE_OUTPUT_DIR)

.PHONY: simple big show clean always

default: simple tree

all: simple tree big

simple:
	OUTPUT_DIR="$(OUTPUT_DIR)/simple" PYTHON="$(PYTHON)" PYTHONFLAGS="$(PYTHONFLAGS)" EMITTER="$(EMITTER)" ./simpletest.sh

big:
	OUTPUT_DIR="$(OUTPUT_DIR)/big" PYTHON="$(PYTHON)" PYTHONFLAGS="$(PYTHONFLAGS)" EMITTER="$(EMITTER)" ./bigtest.sh

tree: tree-aligned $(OUTPUT_FILES) test

$(TREE_OUTPUT_DIR)/%.h $(TREE_OUTPUT_DIR)/%.cpp : %.clad always
	mkdir -p $(TREE_OUTPUT_DIR)/$(shell dirname $*.clad)
	$(CLAD) -C $(TREE_INPUT_DIR) -o $(TREE_OUTPUT_DIR) $*.clad

$(TEST_BIN): $(OUTPUT_CPP) $(OUTPUT_TYPE_CPP) $(CPP_TEST_SRC)
	$(CXX) $(CXXFLAGS) -I$(TREE_SUPPORT_DIR)/include -I$(TREE_OUTPUT_DIR) $(CLAD_SUPPORT_SRC) -o $@ $^

test: $(TEST_BIN) always
	$(TEST_BIN)

show:
	@echo $(INPUT_FILES) $(ALIGNED_INPUT_FILES)

tree-aligned: $(ALIGNED_OUTPUT_FILES)

$(ALIGNED_TREE_OUTPUT_DIR)/%.h $(ALIGNED_TREE_OUTPUT_DIR)/%.cpp : %.clad always
	mkdir -p $(ALIGNED_TREE_OUTPUT_DIR)/$(shell dirname $*.clad)
	$(CLAD) -C $(ALIGNED_TREE_INPUT_DIR) -o $(ALIGNED_TREE_OUTPUT_DIR) $*.clad

clean:
	rm -rf $(OUTPUT_DIR)

