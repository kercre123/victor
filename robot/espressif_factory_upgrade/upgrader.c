/**
 * @file Self destructing factory firmware upgrade program
 * @author Daniel Casner
 */

#include "../espressif_bootloader/cboot-private.h"
#include "../espressif_bootloader/cboot-rtc.h"
#include "../espressif_bootloader/flash_map.h"

extern void SelectSpiFunction();
extern uint32 SPILock();
extern uint32 SPIUnlock();
extern uint32_t SPIParamCfg(uint32_t deviceId, uint32_t chip_size, uint32_t block_size, uint32_t sector_size, uint32_t page_size, uint32_t status_mask);



#define READ_PERI_REG(addr) (*((volatile uint32_t *)(addr)))
#define WRITE_PERI_REG(addr, val) (*((volatile uint32_t *)(addr))) = (uint32_t)(val)
#define REG_SPI_BASE(i) (0x60000200 - i * 0x100)
#define SPI_CMD_REG(i) (REG_SPI_BASE(i) + 0x0)
#define SPI_FLASH_RDID (1<<28)
#define SPI_W0_REG(i) (REG_SPI_BASE(i) + 0x40)

#define FLASH_BLOCK_SIZE 65536
#define FLASH_SECTOR_SIZE 4096
#define FLASH_PAGE_SIZE 256
#define FLASH_STATUS_MASK 0xFFFF


static uint32_t magic = 42; // To keep the toolchain from falling apart

uint32_t wordcmp(const uint32_t* a, const uint32_t* b, const uint32_t size)
{
  uint32_t index;
  for (index = 0; index<size; ++index)
  {
    uint32_t diff = a[index] - b[index];
    if (diff != 0) return diff;
  }
  return 0;
}

// assembler stub uses no stack space
// works with gcc
void call_user_start() {
  uint32_t srcBuffer[SECTOR_SIZE/sizeof(uint32_t)];
  uint32_t dstBuffer[SECTOR_SIZE/sizeof(uint32_t)];
  uint32_t codeRegion = 0;
  uint32_t codeSector = 0;
  uint32_t payloadSize = 0xFFFFffff;
  uint32_t payloadSectors = 0;
  uint32_t sector;
  uint32_t chip_id;

  uart_div_modify(0, (50*1000000)/230400);
  SelectSpiFunction();
  // Debugging delay
  ets_delay_us(1000);
  
  ets_printf("\r\nYou will be upgraded\r\n");

  WRITE_PERI_REG(SPI_CMD_REG(0), SPI_FLASH_RDID);
  while (READ_PERI_REG(SPI_CMD_REG(0)) & SPI_FLASH_RDID) {
  }
  chip_id = READ_PERI_REG(SPI_W0_REG(0)) & 0xFFFFFF;
  
  ets_printf("ChipID %x\r\n", chip_id);
  if (((chip_id >> 16)%0xFF) == 0x16) {
     ets_printf("32Mbit\r\n");
     SPIParamCfg(0, 32*1024*1024, FLASH_BLOCK_SIZE, FLASH_SECTOR_SIZE,
                 FLASH_PAGE_SIZE, FLASH_STATUS_MASK);     
  }
  else {
     ets_printf("16Mbit\r\n");
     SPIParamCfg(0, 16*1024*1024, FLASH_BLOCK_SIZE, FLASH_SECTOR_SIZE,
                 FLASH_PAGE_SIZE, FLASH_STATUS_MASK);
  }
     
  
  if (SPIUnlock() != 0)
  {
    ets_printf("Couldn't unlock SPI flash interface, aborting\r\n");
    return;
  }

  if (system_rtc_mem(RTC_IMAGE_SELECTION, &codeRegion, 1, FALSE) == FALSE)
  {
    ets_printf("Failed to get executing code region, aborting\r\n");
    return;
  }
  switch (codeRegion)
  {
    case FW_IMAGE_A:
      codeSector = APPLICATION_A_SECTOR;
      break;
    case FW_IMAGE_B:
      codeSector = APPLICATION_B_SECTOR;
      break;
    default:
      ets_printf("Invalid code region 0x%x, aborting\r\n", codeRegion);
      return;
  }
  
  SpiFlashOpResult rslt = SPIRead(((codeSector + 1) * SECTOR_SIZE) - 4, &payloadSize, 4);
  if (rslt != SPI_FLASH_RESULT_OK)
  {
    ets_printf("Couldn't read payload size: %d\r\n", rslt);
    return;
  }
  else if ((payloadSize < 200000) || (payloadSize > 524288))
  {
    ets_printf("Invalid payload size, %d, read from 0x%x, aborting\r\n", payloadSize, ((codeSector + 1) * SECTOR_SIZE) - 4);
    return;
  }
  else
  {
    payloadSectors = (payloadSize + SECTOR_SIZE - 1) / SECTOR_SIZE;
    ets_printf("Have %d bytes / %d sectors of payload to write\r\n", payloadSize, payloadSectors);
  }
  
  for (sector = 0; sector<payloadSectors; ++sector)
  {
    const uint32_t dstSector = FACTORY_WIFI_FW_SECTOR + sector;
    const uint32_t srcSector = codeSector + 1 + sector;
    if (SPIRead(dstSector * SECTOR_SIZE, dstBuffer, SECTOR_SIZE) != SPI_FLASH_RESULT_OK)
    {
      ets_printf("Error reading dst flash from 0x%x\r\n", dstSector);
      continue;
    }
    if (SPIRead(srcSector * SECTOR_SIZE, srcBuffer, SECTOR_SIZE) != SPI_FLASH_RESULT_OK)
    {
      ets_printf("Error reading src flash from 0x%x\r\n", srcSector);
      continue;
    }
    
    if (wordcmp(dstBuffer, srcBuffer, SECTOR_SIZE/sizeof(uint32_t)))
    {
      if (SPIEraseSector(dstSector) != SPI_FLASH_RESULT_OK)
      {
        ets_printf("Error erasing sector 0x%x\r\n", dstSector);
        continue;
      }
      else if (SPIWrite(dstSector * SECTOR_SIZE, srcBuffer, SECTOR_SIZE) != SPI_FLASH_RESULT_OK)
      {
        ets_printf("Error writing to 0x%x\r\n", dstSector);
        continue;
      }
      else
      {
        ets_printf("Updated 0x%x\r\n", dstSector);
      }
    }
    else
    {
      ets_printf("Skipping 0x%x, already matches 0x%x\r\n", dstSector, srcSector);
    }
  }
  
  
  ets_printf("Finished installing upgrade\r\n");
  ets_printf("Erasing other app...\r\n"); // Destroy other app first in case we don't completely erase ourselves
  const uint32_t otherAppSector = codeSector == APPLICATION_A_SECTOR ? APPLICATION_B_SECTOR : APPLICATION_A_SECTOR;
  while (SPIEraseSector(otherAppSector)) ets_printf("Trouble erasing sector 0x%x\r\n", otherAppSector);
  ets_printf("Self destructing\r\n");
  while (SPIEraseSector(codeSector) != SPI_FLASH_RESULT_OK) ets_printf("Trouble erasing sector 0x%x\r\n", codeSector);
  ets_printf("Done\r\n");
  magic = 0;
}
