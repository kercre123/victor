#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hal/board.h"
#include "hal/display.h"
#include "hal/flash.h"
#include "hal/monitor.h"
#include "hal/motorled.h"
#include "hal/portable.h"
#include "hal/radio.h"
#include "hal/timers.h"
#include "hal/testport.h"
#include "hal/uart.h"
#include "hal/console.h"
#include "hal/cube.h"
#include "app/fixture.h"
#include "hal/espressif.h"
#include "hal/random.h"
#include "app/app.h"
#include "app/tests.h"
#include "nvReset.h"

//derived from reverse-engineered ROM function: https://blog.cesanta.com/flashing-esp8266-too-many-erased-sectors
//return - number of sectors erased by esp ROM func
static int SPIEraseArea_sectorCount(int32_t addr, int32_t size)
{
  const int fd_flash_size   = 0x200000;
  const int fd_block_size   = 0x010000; //64k
  const int fd_sector_size  = 0x001000; //4k
  int sectors_erased = 0;
  
  if (fd_flash_size < addr + size)
    return sectors_erased; //return 1;
  if (addr % fd_sector_size != 0)
    return sectors_erased; //return 1;

  addr /= fd_sector_size; //convert to sector #
  int32_t sectors_per_block = fd_block_size / fd_sector_size;
  int32_t sectors_to_erase = size / fd_sector_size;
  if (size % fd_sector_size != 0)
    sectors_to_erase++;
  size = sectors_to_erase;
  
  if (sectors_per_block - (addr % sectors_per_block) < sectors_to_erase)
    size = sectors_per_block - (addr % sectors_per_block);
  while (size != 0) {
    sectors_erased++; //if (SPIEraseSector(addr)) return 1;
    addr++;
    size--;
  }
  
  size = sectors_to_erase - size;  // effectively, size = sectors_to_erase
  while (size > sectors_per_block) {  // should be >=, but okay
    sectors_erased += sectors_per_block; //if (SPIEraseBlock(addr / sectors_per_block)) return 1;
    addr += sectors_per_block;
    size -= sectors_per_block;
  }
  
  while (size != 0) {
    sectors_erased++; //if (SPIEraseSector(addr)) return 1;
    addr++;
    size--;
  }
  
  return sectors_erased;
}

//from esptool.py __version__ = "2.0-beta1"
//@return 'corrected' erase size for esp ROM flash-begin
static int esptool_get_erase_size(u32 address, int size)
{
  //Breif: ESP8266 has a ROM bug, 'Flash Begin' erases more sectors than specified.
  //  https://blog.cesanta.com/flashing-esp8266-too-many-erased-sectors
  //  workaround from esptool.py (__version__ = "2.0-beta1")
  //NOTE: can't prevent ROM from, sometimes, extending to even # sector for erase.
  
  //def get_erase_size(self, offset, size):
  const int sectors_per_block = 16;
  const int sector_size = 0x1000;
  int num_sectors = (size + sector_size - 1) / sector_size;
  int start_sector = address / sector_size;

  int head_sectors = sectors_per_block - (start_sector % sectors_per_block);
  if( num_sectors < head_sectors )
      head_sectors = num_sectors;

  if( num_sectors < 2 * head_sectors )
      return (num_sectors + 1) / 2 * sector_size;
  else
      return (num_sectors - head_sectors) * sector_size;
}

//https://blog.cesanta.com/flashing-esp8266-too-many-erased-sectors
//x – number of sectors we want to erase
//t – number of sectors to the end of the first block (that is, number of sectors in the block if we start at the block boundary)
static int Cesanta_f_x_t(int x_num_sectors_to_erase, int t_sectors_to_next_block)
{
  //return - number of sectors that will get erased
  return (x_num_sectors_to_erase <= t_sectors_to_next_block)
    ? 2*t_sectors_to_next_block
    : x_num_sectors_to_erase + t_sectors_to_next_block;
}
static int Cesanta_g_x_t(int x_num_sectors_to_erase, int t_sectors_to_next_block)
{
  //return - corrected x-input to f(x,t)
  #define DIV_ROUND_UP(number,divisor)  ((number + divisor - 1) / divisor)
  return (x_num_sectors_to_erase <= 2*t_sectors_to_next_block)
    ? DIV_ROUND_UP(x_num_sectors_to_erase, 2)
    : x_num_sectors_to_erase - t_sectors_to_next_block;
}

bool DebugTestDetectDevice(void)
{
  return true;
}

void DebugInit(void)
{
}

void DebugProcess(void)
{
  const int flash_size   = 0x200000;
  const int block_size   = 0x010000;
  const int sector_size  = 0x001000;
  const int sectors_per_block = block_size/sector_size;
  const int max_rom_size = 0x048000;
  
  //Some starting addresses to test (actual ESP bin addresses)
  const int addr_test[] = { 0x000000, 0x001000, 0x080000, 0x0c8000, 0x1fc000 };
  const int addr_num = sizeof(addr_test)/sizeof(int);
  
  for( int i=0; i < addr_num; i++ )
  {
    int addr = addr_test[i];
    int sectors_to_next_block = sectors_per_block - ((addr/sector_size/*sector#*/) % sectors_per_block);
    
    for( int size = 1; size <= max_rom_size && addr+size < flash_size; size += sector_size )
    {
      int sectors = (size + sector_size - 1) / sector_size; //actual data # sectors (max)
      int sectors_rom  = SPIEraseArea_sectorCount(addr, size                              ); //(predicted) ROM sector erase
      int sectors_espt = SPIEraseArea_sectorCount(addr, esptool_get_erase_size(addr,size) ); //(predicted) ROM sector erase with corrected size input [esptool]
      int sectors_fxt  = Cesanta_f_x_t(sectors,                                       sectors_to_next_block); //(predicted) ROM sector erase
      int sectors_fgxt = Cesanta_f_x_t(Cesanta_g_x_t(sectors, sectors_to_next_block), sectors_to_next_block); //(predicted) ROM sector erase with corrected size input [g(x,t)]
      
      ConsolePrintf("%06X %05X erase# rom:fxt %02X:%02X/%02X (%2d:%2d) espt:fgxt %02X:%02X/%02X (%2d:%2d)\r\n",
                    addr,size,
                    sectors_rom,  sectors_fxt,  sectors, sectors_rom-sectors,  sectors_fxt-sectors,
                    sectors_espt, sectors_fgxt, sectors, sectors_espt-sectors, sectors_fgxt-sectors );
    }
  }
  
  g_fixtureType = FIXTURE_NONE;
}

TestFunction* GetDebugTestFunctions()
{
  static TestFunction m_debugFunctions[] = 
  {
    DebugInit,
    DebugProcess,
    NULL
  };
  return m_debugFunctions;
}

