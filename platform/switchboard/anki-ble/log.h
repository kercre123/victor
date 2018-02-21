/**
 * File: log.h
 *
 * Author: seichert
 * Created: 1/10/2018
 *
 * Description: log functions for Anki Bluetooth Daemon
 *
 * Copyright: Anki, Inc. 2018
 *
 **/

#pragma once

#include <cstdarg>

const int kLogLevelVerbose = 2;
const int kLogLevelDebug = 3;
const int kLogLevelInfo = 4;
const int kLogLevelWarn = 5;
const int kLogLevelError = 6;

#ifdef __cplusplus
extern "C" {
#endif

bool isUsingAndroidLogging();
void enableAndroidLogging(const bool enable);
void setAndroidLoggingTag(const char* tag);
int getMinLogLevel();
void setMinLogLevel(const int level);

void logv(const char* fmt, ...);
void logd(const char* fmt, ...);
void logi(const char* fmt, ...);
void logw(const char* fmt, ...);
void loge(const char* fmt, ...);

#ifdef __cplusplus
}
#endif