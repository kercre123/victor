#ifndef SERIAL_SUPPORT_H__
#define SERIAL_SUPPORT_H__


#define platform_set_baud(fd, cfg, speed) \
   cfsetispeed(&cfg, speed), cfsetospeed(&cfg, speed)


#endif//SERIAL_SUPPORT_H__
