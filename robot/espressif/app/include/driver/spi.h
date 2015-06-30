#ifndef SPI_APP_H
#define SPI_APP_H

#include "spi_register.h"
#include "ets_sys.h"
#include "osapi.h"
#include "uart.h"
#include "os_type.h"
#include "spi_flash.h"

#define SPI_FLASH_BYTES_LEN                24
#define IODATA_START_ADDR                 BIT0
#define SPI_BUFF_BYTE_NUM                    32

// SPI number define
typedef enum {
  SPI  = 0, ///< Shared with program flash
  HSPI = 1, ///< Independent SPI bus
  NUM_SPI_BUS = 2; ///< Number of buses in system
} SPIBus;

/** Initalize the specified SPI peripheral for master mode
 * @param spi_no Which SPI bus to initalize
 * @param frequency SPI clock frequency to configure for in HZ
 * @return The frequency actually selected
 */
uint32 spi_master_init(const SPIBus spi_no, uint32 frequency);

/** Setup an SPI transaction to be followed by data
 * @param spi_no Which SPI bus to use
 * @param cmd_bits The number of bits of command data @range 0-16
 * @param cmd_data Command data, only the lower cmd_bits of data will be used
 * @param addr_bits The number of bits of address data @rante 0-32
 * @param addr_data Address data, only the lower addr_bits will be used.
 * @param mosi_bits The number of bits of master out / slave in data to transmit @range 0-512
 * @param miso_bits The number of bits of master in / slave out data to receive. @range 0-512
 * @param dummy_bits The number of dummy bits (extra clock cycles) to introduce. If miso_bits is > 0 then the dummy bits
 *                   will be between the mosi_data and the miso_data. Otherwise it will be between the address data and
 *                   the mosi_data. @range 0-255
 */
void spi_mast_start_transaction(const SPIBus spi_no, const uint8 cmd_bits, const uint16 cmd_data, \
                                const uint8 addr_bits, const uint32 addr_data, \
                                const uint16 mosi_bits, const uint16 miso_bits, const uint16 dummy_bits);

/** Queue a 32 bit word for transmission during the mosi phase of an SPI transaction
 * @param spi_no Which SPI bus to use
 * @param data A 32 bit words to transmit.
 * @return true if the word was added to the FIFO. False if there was no room
 */
bool spi_mast_write(const SPIBus spi_no, const uint32 data);

/** Read a received 32 bit word from the FIFO
 * @param spi_no Which SPI bus to use
 * @return The read word. @note if the FIFO is empty 0 will be returned.
 */
uint32 spi_mast_read(const SPIBus spi_no);

#ifdef 0
//esp8266 slave mode initial
void spi_slave_init(uint8 spi_no,uint8 data_len);
//transmit data to esp8266 slave buffer,which needs 16bit transmission ,
//first byte is master command 0x04, second byte is master data
void spi_byte_write_espslave(uint8 spi_no,uint8 data);
//read data from esp8266 slave buffer,which needs 16bit transmission ,
//first byte is master command 0x06, second byte is to read slave data
void spi_byte_read_espslave(uint8 spi_no,uint8 *data);

//esp8266 slave isr handle funtion,tiggered when any transmission is finished.
//the function is registered in spi_slave_init.
void spi_slave_isr_handler(void *para);
#endif

#endif
