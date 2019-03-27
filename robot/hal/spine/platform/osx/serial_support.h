#ifndef SERIAL_SUPPORT_H__
#define SERIAL_SUPPORT_H__


#include <sys/ioctl.h>
#include <IOKit/serial/ioss.h>
#include <string.h>

#define platform_set_baud(fd, cfg, _speed) do{\
      speed_t speed = _speed;\
      if (ioctl(fd, IOSSIOSPEED, &speed) == -1) {\
         LOGE("Error calling ioctl(..., IOSSIOSPEED, %ld) %s(%d).\n", speed, strerror(errno), errno); \
      }} while (0)

#endif//SERIAL_SUPPORT_H__


