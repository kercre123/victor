#ifndef PLATFORM_LOGGING_H
#define PLATFORM_LOGGING_H

#include "android/log.h"
static const char *TAG="spine";
#define LOGI(fmt, args...) __android_log_print(ANDROID_LOG_INFO,  TAG, fmt, ##args)
#define LOGD(fmt, args...) __android_log_print(ANDROID_LOG_DEBUG, TAG, fmt, ##args)
#define LOGE(fmt, args...) __android_log_print(ANDROID_LOG_ERROR, TAG, fmt, ##args)


#endif//PLATFORM_LOGGING_H


