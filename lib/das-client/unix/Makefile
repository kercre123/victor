DAS_TESTS_SRC_FILES = \
	$(DAS_TESTING_DIR)/dasLogFileAppender_unittest.cpp \
	$(DAS_TESTING_DIR)/dasGameLogAppender_unittest.cpp \
	$(DAS_TESTING_DIR)/dasAppender_unittest.cpp \
	$(DAS_TESTING_DIR)/das_unittest.cpp \
	$(DAS_TESTING_DIR)/testUtils.cpp

DAS_TEST_OBJ_FILES := $(patsubst %.cpp, %.o, $(notdir $(DAS_TESTS_SRC_FILES)))
DAS_TEST_DEP_FILES := $(patsubst %.cpp, %.d, $(notdir $(DAS_TESTS_SRC_FILES)))

.PHONY : all
all: dasDemoApp das_unittest run-unit-tests-valgrind

include make.mk

.PHONY: run-unit-tests
run-unit-tests: das_unittest
	./das_unittest

.PHONY: run-unit-tests-valgrind
run-unit-tests-valgrind: das_unittest misc.sup
	valgrind --gen-suppressions=all --suppressions=misc.sup --leak-check=full --show-leak-kinds=all ./das_unittest
