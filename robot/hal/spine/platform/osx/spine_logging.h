#ifndef PLATFORM_LOGGING_H
#define PLATFORM_LOGGING_H



#include <stdio.h>

#define LOGI(fmt, args...) printf("Inf: " fmt "\n", ##args)
#define LOGD(fmt, args...) printf("Dbg: " fmt "\n", ##args)
#define LOGE(fmt, args...) printf("Err: " fmt "\n", ##args)

#endif//PLATFORM_LOGGING_H


