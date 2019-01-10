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
#if defined(HWVER_REVG)
  #define HWVERS "REVG"
#elif defined(HWVER_REVF)
  #define HWVERS "REVF"
#elif defined(HWVER_REVD)
  #define HWVERS "REVD"
#elif defined(HWVER_DVT3A)
  #define HWVERS "DVT3A"
#elif defined(HWVER_DVT2B)
  #define HWVERS "DVT2B"
#else
  #define HWVERS "?"
#endif

//-----------------------------------------------------------
//        API
//-----------------------------------------------------------

#ifdef __cplusplus

//channel args for Board::getAdcMv()
enum adc_chan_e {
  ADC_VEXT_SENSE  = 2,
  ADC_NTC         = 3,
  ADC_VIN_SENSE   = 4,
  ADC_TEMP_CPU    = 16, //internal Temperature sensor
  ADC_VREFINT     = 17, //internal Vref (bandgap)
};

namespace Board
{
  void init();
  
  //power ctrl
  void pwr_vdd(bool en);  //VBATs
  void pwr_vdds(bool en); //vdds (VENC) sensor rail
  void pwr_vmain(bool en); //VMAIN enable
  void pwr_charge(bool en); //VEXT connect
  
  uint32_t adcReadRaw(adc_chan_e chan); //raw adc value
  uint32_t adcRead(adc_chan_e chan, int oversample = 0); //adc value, VDD compensated. 2^oversample = # samples (averaged)
  uint32_t adcReadMv(adc_chan_e chan, int oversample = 0); //2^oversample = # samples (averaged)
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
//namespace DBGLED GPIO_DEFINE(B, 7); //BODY_RX
#endif /* __cplusplus */


#endif //__BOARD_H

