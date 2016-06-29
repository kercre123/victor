#ifndef __FLASHER_ERROR_H_
#define __FLASHER_ERROR_H_

enum {
  FLASHER_SUCCESS = '0',       ///< Success
  FLASHER_BAD_COMMAND = '1',   ///< Invalid UART command
  FLASHER_ERROR = '2',         ///< SPI flash error
  FLASHER_TIMEOUT,             ///< SPI flash timeout
  FLASHER_UNEXPECTED,          ///< Unexpected SPI flash result
  FLASHER_READBACK_ERROR,      ///< SPI flash error while reading back after successfully writing
  FLASHER_READBACK_TIMEOUT,    ///< SPI flash timeout while reading back after successfully writing
  FLASHER_READBACK_UNEXPECTED, ///< Unexpected SPI flash result while reading back after successfully writing.
  FLASHER_SPI_FATAL = '9'      ///< Failed to initalize SPI flash interface, flasher is useless
};

#endif
