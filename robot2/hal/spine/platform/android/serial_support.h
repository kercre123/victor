#ifndef SERIAL_SUPPORT_H__
#define SERIAL_SUPPORT_H__


#define platform_set_baud(fd, cfg, speed) \
   cfsetispeed(&cfg, baudrate), cfsetospeed(&cfg, baudrate)


#endif//SERIAL_SUPPORT_H__
