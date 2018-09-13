SUFFIXES += .d
DAS_PROJECT_PATH = ..
DAS_SRC_DIR = $(DAS_PROJECT_PATH)/src
JSONCPP_SRC_DIR = $(DAS_PROJECT_PATH)/../util/source/3rd/jsoncpp
DAS_INC_DIR = $(DAS_PROJECT_PATH)/include
DAS_TESTING_DIR = $(DAS_PROJECT_PATH)/testing
GTEST_DIR = $(DAS_TESTING_DIR)/gtest

DAS_SRC_FILES := \
	$(JSONCPP_SRC_DIR)/jsoncpp.cpp \
	$(DAS_SRC_DIR)/DAS.cpp \
	$(DAS_SRC_DIR)/dasAppender.cpp \
	$(DAS_SRC_DIR)/dasLogFileAppender.cpp \
	$(DAS_SRC_DIR)/dasGameLogAppender.cpp \
	$(DAS_SRC_DIR)/dasFilter.cpp \
	dasLocalAppenderImpl.cpp \
	dasLocalAppenderFactory.cpp \
	dasPostToServer.cpp \
	$(DAS_SRC_DIR)/stringUtils.cpp \
	$(DAS_SRC_DIR)/fileUtils.cpp \
	$(DAS_SRC_DIR)/taskExecutor.cpp

vpath %.cpp $(DAS_SRC_DIR) $(DAS_TESTING_DIR)

CXXFLAGS += -g
CXXFLAGS += -I$(DAS_INC_DIR)
CXXFLAGS += -I$(DAS_INC_DIR)/DAS
CXXFLAGS += -I$(JSONCPP_SRC_DIR)
CXXFLAGS += -I$(DAS_SRC_DIR)
CXXFLAGS += -I.
CXXFLAGS += -fdiagnostics-show-category=name
CXXFLAGS += -fexceptions
CXXFLAGS += -Wall
CXXFLAGS += -Woverloaded-virtual
CXXFLAGS += -Werror
CXXFLAGS += -Wno-extern-c-compat
CXXFLAGS += -Wno-sentinel
CXXFLAGS += -Wno-return-type-c-linkage
CXXFLAGS += -Wno-tautological-compare
CXXFLAGS += -Wno-parentheses-equality
CXXFLAGS += -Wno-static-in-inline
CXXFLAGS += -std=c++14
CXXFLAGS += -pthread

# Flags passed to the preprocessor.
# Set Google Test's header directory as a system directory, such that
# the compiler doesn't generate warnings in Google Test headers.
CPPFLAGS += -isystem $(GTEST_DIR)/include

# All Google Test headers.  Usually you shouldn't change this
# definition.
GTEST_HEADERS = $(GTEST_DIR)/include/gtest/*.h \
                $(GTEST_DIR)/include/gtest/internal/*.h


DAS_OBJ_FILES := $(patsubst %.cpp, %.o, $(DAS_SRC_FILES))
DAS_DEP_FILES := $(patsubst %.cpp, %.d, $(DAS_SRC_FILES))

dasDemoApp: dasDemoApp.cpp dasLib.a $(DAS_INC_DIR)/DAS/DAS.h
	$(CXX) $(CXXFLAGS) -o dasDemoApp dasDemoApp.cpp dasLib.a $(CXXLIBS)

dasLib.a: $(DAS_OBJ_FILES)
	$(AR) $(ARFLAGS) $@ $^

# Builds gtest.a and gtest_main.a.

# Usually you shouldn't tweak such internal variables, indicated by a
# trailing _.
GTEST_SRCS_ = $(GTEST_DIR)/src/*.cc $(GTEST_DIR)/src/*.h $(GTEST_HEADERS)

# For simplicity and to avoid depending on Google Test's
# implementation details, the dependencies specified below are
# conservative and not optimized.  This is fine as Google Test
# compiles fast and for ordinary users its source rarely changes.
gtest-all.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest-all.cc

gtest_main.o : $(GTEST_SRCS_)
	$(CXX) $(CPPFLAGS) -I$(GTEST_DIR) $(CXXFLAGS) -c \
            $(GTEST_DIR)/src/gtest_main.cc

gtest.a : gtest-all.o
	$(AR) $(ARFLAGS) $@ $^

gtest_main.a : gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

das_unittest : dasLib.a $(DAS_TEST_OBJ_FILES) gtest_main.a
	$(CXX) -g  $^ -o $@ $(CXXLIBS)

%.d : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT '$(patsubst %.cpp,%.o, $(notdir $<))' $< -MF $@

%.o : %.cpp %.d make.mk Makefile Makefile_sqs
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<


clean:
	rm -rf dasDemoApp dasLib.a *.o *.d dasDemoApp.dSYM dasLogs gameLogs das_unittest gtest.a gtest_main.a

NODEPS:=clean
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
	-include $(DAS_DEP_FILES)
	-include $(DAS_TEST_DEP_FILES)
endif
