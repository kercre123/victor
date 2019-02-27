#ifndef PLATFORM_LOGGING_H
#define PLATFORM_LOGGING_H

#include <android/log.h>
static const char *TAG = "vic-spine";
#define LOGI(fmt, args...) printf(fmt, ##args)
#define LOGD(fmt, args...) printf(fmt, ##args)
#define LOGE(fmt, args...) printf(fmt, ##args)
#define LOGW(fmt, args...) printf(fmt, ##args)


#endif//PLATFORM_LOGGING_H
