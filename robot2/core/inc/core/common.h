#ifndef CORE_COMMON_H
#define CORE_COMMON_H

#include <stdio.h>

#define DEBUG_CORE
#ifdef DEBUG_CORE
#define dprintf printf
#else
#define dprintf(s, ...)
#endif



typedef enum CoreAppErrorCode_t {
  app_SUCCESS = 0,
  app_USAGE = -1,
  app_FILE_OPEN_ERROR = -2,
  app_FILE_READ_ERROR = -3,
  app_SEND_DATA_ERROR = -4,
  app_INIT_ERROR = -5,
  app_FLASH_ERASE_ERROR = -6,
  app_VALIDATION_ERROR = -7,
  app_FILE_SIZE_ERROR = -8,
  app_MEMORY_ERROR = -9,
  app_IO_ERROR = -10,
} CoreAppErrorCode;


/// Applications must implement this
extern void error_exit(CoreAppErrorCode, const char*, ...);

/* #define core_set_error_handler(error_exit) core_error = (error_exit) */


#endif//CORE_COMMON_H
