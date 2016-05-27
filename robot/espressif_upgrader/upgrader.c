/*******************************************************************************
 * @file Bootloader and factory image upgrader
 * @copyright Anki, 2016
 * @author Daniel Casner <daniel@anki.com>
 *
 */

typedef int int32;
typedef int int32_t;
typedef unsigned int uint32;
typedef unsigned int uint32_t;
typedef unsigned char uint8;
typedef unsigned char uint8_t;

#define NOINLINE __attribute__ ((noinline))

#define CRYSTAL_FREQ 26000000
#define CPU_CLK_FREQ (80*(CRYSTAL_FREQ)/16000000)
#define UART_CLK_FREQ (80*1000000)

#define ROM_MAGIC	   0xe9
#define ROM_MAGIC_NEW1 0xea
#define ROM_MAGIC_NEW2 0x04

#define CHKSUM_INIT 0xef

#define TRUE 1
#define FALSE 0
#define NULL 0

extern uint32 SPIRead(uint32 addr, void *outptr, uint32 len);
extern uint32 SPIEraseSector(int);
extern uint32 SPIWrite(uint32 addr, void *inptr, uint32 len);
extern void ets_printf(char*, ...);
extern void ets_delay_us(int);
extern void ets_memset(void*, uint8, uint32);
extern void ets_memcpy(void*, const void*, uint32);
extern void uart_div_modify(int no, int freq);

typedef enum {
  SPI_FLASH_RESULT_OK,
  SPI_FLASH_RESULT_ERR,
  SPI_FLASH_RESULT_TIMEOUT,
} SpiFlashOpResult;


#define SECTOR_SIZE (0x1000)

#define NEW_BOOTLOADER_ADDRESS   (0x101000)
#define NEW_FIRMWARE_SRC_ADDRESS (0x102000)
#define NEW_FIRMWARE_DST_SECTOR  (0x2)

/** Sets up the UART so we can get debugging output from the bootloader
 */
void NOINLINE setupSerial(void)
{
  // Update the clock rate here since it's the first function we call
  uart_div_modify(0, (50*1000000)/230400);
  // Debugging delay
  ets_delay_us(2000000);
  
  ets_printf("\r\n\r\nYou will be upgraded\r\n\r\n");
}

void spi_read_wrap(uint32 src, uint32* dst, uint32 len)
{
  while (SPIRead(src, dst, len) != SPI_FLASH_RESULT_OK)
  {
    ets_printf("Trouble reading from 0x%x\r\n", src);
  }
}

void spi_write_wrap(uint32 dst, uint32* src, uint32 len)
{
  while (SPIWrite(dst, src, len) != SPI_FLASH_RESULT_OK)
  {
    ets_printf("Trouble writing to 0x%x\r\n", dst);
  }
}

void spi_erase_wrap(uint32 sector)
{
  while (SPIEraseSector(sector) != SPI_FLASH_RESULT_OK)
  {
    ets_printf("Trouble erasing secotr 0x%x\r\n", sector);
  }
}

void copy_sector(uint32 srcAddress, uint32 dstSector)
{
  uint32 buffer[SECTOR_SIZE/sizeof(uint32)];
  ets_printf("Copy 0x%x to 0x%x\r\n\tReading...\r\n", srcAddress, dstSector * SECTOR_SIZE);
  spi_read_wrap(srcAddress, buffer, SECTOR_SIZE);
  ets_printf("\tErasing...\r\n");
  spi_erase_wrap(dstSector);
  ets_printf("\tWriting...\r\n");
  spi_write_wrap(dstSector * SECTOR_SIZE, buffer, SECTOR_SIZE);
}

void call_user_start() {
  uint32 firmwareSize = 0;
  uint32 sectorCount  = 0;
  
  setupSerial();
  
  spi_read_wrap(NEW_FIRMWARE_SRC_ADDRESS, &firmwareSize, sizeof(uint32));
  
  ets_printf("Have 0x%x bytes of firmware to copy over\r\n", firmwareSize);
  
  // Skip overwriting the sectors we're executing from for first pass
  for (sectorCount = 2; sectorCount < ((firmwareSize/SECTOR_SIZE) + 1); sectorCount++)
  {
    copy_sector(NEW_FIRMWARE_SRC_ADDRESS + 4 + sectorCount*SECTOR_SIZE, NEW_FIRMWARE_DST_SECTOR + sectorCount);
  }
  
  ets_printf("Entering unrecoverable zone\r\n");
  
  copy_sector(NEW_FIRMWARE_SRC_ADDRESS + 4 + 0*SECTOR_SIZE, NEW_FIRMWARE_DST_SECTOR + 0);
  copy_sector(NEW_FIRMWARE_SRC_ADDRESS + 4 + 1*SECTOR_SIZE, NEW_FIRMWARE_DST_SECTOR + 1);
  copy_sector(NEW_BOOTLOADER_ADDRESS, 0);
  
  ets_printf("Upgrade complete\r\n");
}
