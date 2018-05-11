#include <stdint.h>

#ifndef CORE_SERIAL_H
#define CORE_SERIAL_H

/// Opens serial port at `devicename` with given `baud` enum.
/// Returns file descriptor.
int serial_init(const char* devicename, int baud);

/// Writes `len` bytes from `buffer` to port `serial_fd`.
/// Returns count of bytes written.
int serial_write(int serial_fd, const uint8_t* buffer, int len);


/// Attempts to read `len` bytes into `buffer` from port `serial_fd`.
/// Non-blocking.
/// Returns count of bytes recieved, or <0 on error.
int serial_read(int serial_fd, uint8_t* buffer, int len);

/// closes open port at `serialFd`
void serial_close(int serialFd);

#endif//CORE_SERIAL_H
