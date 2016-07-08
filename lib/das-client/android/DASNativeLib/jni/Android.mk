# Copyright (C) 2009 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DAS_PROJECT_PATH:= $(realpath $(dir $(call this-makefile)))
LOCAL_PATH := $(DAS_PROJECT_PATH)
include $(call all-subdir-makefiles)

DAS_SRC_FILES := \
	../../../src/DAS.cpp \
	../../../src/dasAppender.cpp \
	../../../src/dasFilter.cpp \
	../../../src/dasLogFileAppender.cpp \
	../../../src/dasGameLogAppender.cpp \
	dasLocalAppender.cpp \
	dasJni.cpp \
	dasPlatform_android.cpp \
	../../../src/stringUtils.cpp \
	../../../src/fileUtils.cpp \
	../../../src/taskExecutor.cpp \
	../../../src/jsoncpp.cpp

DAS_C_INCLUDES := \
	$(DAS_PROJECT_PATH)/../../../src \
	$(DAS_PROJECT_PATH)/../../../include \
	$(DAS_PROJECT_PATH)/../../../include/DAS \
	$(DAS_PROJECT_PATH)

DAS_LOCAL_CFLAGS += -fdiagnostics-show-category=name
DAS_LOCAL_CFLAGS += -fexceptions
DAS_LOCAL_CFLAGS += -Wall
DAS_LOCAL_CFLAGS += -Woverloaded-virtual
DAS_LOCAL_CFLAGS += -Werror
DAS_LOCAL_CFLAGS += -Wno-extern-c-compat
DAS_LOCAL_CFLAGS += -Wno-sentinel
DAS_LOCAL_CFLAGS += -Wno-return-type-c-linkage
DAS_LOCAL_CFLAGS += -Wno-tautological-compare
DAS_LOCAL_CFLAGS += -Wno-parentheses-equality
DAS_LOCAL_CFLAGS += -Wno-static-in-inline

include $(CLEAR_VARS)
LOCAL_PATH := $(DAS_PROJECT_PATH)
LOCAL_MODULE := das_static
LOCAL_SRC_FILES := $(DAS_SRC_FILES)
LOCAL_C_INCLUDES += $(DAS_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES) $(LOCAL_PATH)
LOCAL_EXPORT_LDLIBS := -llog
LOCAL_CFLAGS += $(DAS_LOCAL_CFLAGS)
include $(BUILD_STATIC_LIBRARY)


include $(CLEAR_VARS)
LOCAL_PATH := $(DAS_PROJECT_PATH)
LOCAL_MODULE := DAS
LOCAL_SRC_FILES := $(DAS_SRC_FILES)
LOCAL_C_INCLUDES += $(DAS_C_INCLUDES)
LOCAL_EXPORT_C_INCLUDES := $(LOCAL_C_INCLUDES) $(LOCAL_PATH)
LOCAL_LDLIBS := -llog
LOCAL_CFLAGS += $(DAS_LOCAL_CFLAGS)
include $(BUILD_SHARED_LIBRARY)



