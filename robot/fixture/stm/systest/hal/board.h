#ifndef __BOARD_H
#define __BOARD_H

#include <stdint.h>
#include <stdlib.h>
#include "common.h"
#include "stm32f0xx.h"

//-----------------------------------------------------------
//        Board Config
//-----------------------------------------------------------

//hardware version string
#if defined(HWVERP2)
  #define HWVERS "P2"
#elif defined(HWVERP3)
  #define HWVERS "P3"
#else
  #define HWVERS "P?"
#endif

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

//power source for switched rail VBATs
enum vbats_src_e {
  VBATS_SRC_OFF = 0,
  VBATS_SRC_VBAT = 1,
  VBATS_SRC_VEXT = 2
};

//battery charger enable/strength
enum chrg_en_e {
  CHRG_OFF = 0,
  CHRG_LOW = 1, /*low current charging*/
  CHRG_HIGH = 2 /*high current charging*/
};

#ifdef __cplusplus
namespace Board
{
  void init();
  
  //power ctrl
  void vdd(bool en); //mcu regulator enable (keep alive)
  void vdds(bool en); //vdd switched (sensor rail)
  void vbats(vbats_src_e src); //vbat swtiched (head, sensors)
  void charger(chrg_en_e state); //battery charger
}
#endif /* __cplusplus */

//Globals
char* snformat(char *s, size_t n, const char *format, ...);

//-----------------------------------------------------------
//        pin defines
//-----------------------------------------------------------
#ifdef __cplusplus
#include "hardware.h"
#endif /* __cplusplus */


//------------------------------------------------  
//    Hacky debug LED (temporary)
//------------------------------------------------

#ifdef __cplusplus
//P3 Hacked up dev board has an LED tacked onto BODY_RX pin
namespace DBGLED GPIO_DEFINE(B, 7); //BODY_RX
#endif /* __cplusplus */


#endif //__BOARD_H

