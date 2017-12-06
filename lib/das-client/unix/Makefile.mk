SUFFIXES += .d
DAS_PROJECT_PATH = ..
DAS_SRC_DIR = $(DAS_PROJECT_PATH)/src
DAS_INC_DIR = $(DAS_PROJECT_PATH)/include
DAS_TESTING_DIR = $(DAS_PROJECT_PATH)/testing
GTEST_DIR = $(DAS_TESTING_DIR)/gtest

DAS_SRC_FILES := \
	$(DAS_SRC_DIR)/DAS.cpp \
	$(DAS_SRC_DIR)/dasAppender.cpp \
	$(DAS_SRC_DIR)/dasLogFileAppender.cpp \
	$(DAS_SRC_DIR)/dasGameLogAppender.cpp \
	$(DAS_SRC_DIR)/dasFilter.cpp \
	dasLocalAppenderImpl.cpp \
	dasLocalAppenderFactory.cpp \
	$(DAS_SRC_DIR)/stringUtils.cpp \
	$(DAS_SRC_DIR)/fileUtils.cpp \
	$(DAS_SRC_DIR)/taskExecutor.cpp \
	$(DAS_SRC_DIR)/jsoncpp.cpp \

vpath %.cpp $(DAS_SRC_DIR) $(DAS_TESTING_DIR)

CXXFLAGS += -g
CXXFLAGS += -I$(DAS_INC_DIR)
CXXFLAGS += -I$(DAS_INC_DIR)/DAS
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

%.d : %.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT '$(patsubst %.d,%.o, $(notdir $@))' $< -MF $@

%.o : %.cpp %.d Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.d : %.mm
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MM -MT '$(patsubst %.d,%.o, $(notdir $@))' $< -MF $@

%.o : %.mm %.d Makefile
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

%.d : %.m
	$(CC) -c -o '$(patsubst %.m,%.o, $(notdir $<))' $< -MD -MF $@

%.o : %.m %.d Makefile
	$(CC) -c -o '$(patsubst %.m,%.o, $(notdir $<))' $< -MD -MF $@
