#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "eagle_soc.h"
#include "os_type.h"
#include "user_interface.h"
#include "user_config.h"
#include "anki/cozmo/robot/flash_map.h"

extern void Cache_Read_Enable(uint32, uint32, uint32);
/// Overrides SDK Cache read enable
void Cache_Read_Enable_New(void) {
  static uint8 odd_even = 0xFF;
  if (odd_even == 0xFF) // Need to initalize
  {
    uint32 romSelection = 0;
    system_rtc_mem_read(RTC_IMAGE_SELECTION, &romSelection, 4);
    //ets_printf("Overriding Cache for app %08x\r\n", romSelection);
    switch (romSelection)
    {
      case FW_IMAGE_FACTORY:
      case FW_IMAGE_A:
      {
        odd_even = 0;
        break;
      }
      case FW_IMAGE_B:
      {
        odd_even = 1;
        break;
      }
      default:
      {
        //ets_printf("Cache_Read_Enable_New Override, invalid rom selection %x\r\nAssuming A map\r\n", romSelection);
        odd_even = 0;
      }
    }
  }
  
  Cache_Read_Enable(odd_even, 0, 1);
}
