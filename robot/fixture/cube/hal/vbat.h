#ifndef VBAT_H_
#define VBAT_H_

#include <stdint.h>
#include "..\sdk_da1458x\5.0.4\sdk\platform\driver\adc\adc.h" //use dialog sdk adc driver

//-----------------------------------------------------------
//        Defines
//-----------------------------------------------------------

//#define VBAT_RAW_TO_MV(sample_)   (((uint32_t)(sample_)*3600)/1023) //full-scale is 3.6V, 10-bit raw (1023)
#define VBAT_RAW_TO_MV(sample_)   (((uint32_t)(sample_)*3600)>>10) //optimize: div by 1024

//-----------------------------------------------------------
//        Interface (statics)
//-----------------------------------------------------------

//get raw adc value for the 1V or 3V power rail
//@param: channel 0=VBAT3V 1=VBAT1V
static inline uint16_t get_vbat_raw(bool vbat_1v_n3v) {
  adc_calibrate();
  return adc_get_vbat_sample(vbat_1v_n3v) >> 1; //dialog driver sums consective +/- samples for bias offset. /2 for expected 10-bit accuracy.
}

static inline uint16_t get_vbat1v_raw(void) { return get_vbat_raw(1); }
static inline uint16_t get_vbat3v_raw(void) { return get_vbat_raw(0); }

static inline uint16_t get_vbat1v_mv(void) { return VBAT_RAW_TO_MV( get_vbat1v_raw() ); }
static inline uint16_t get_vbat3v_mv(void) { return VBAT_RAW_TO_MV( get_vbat3v_raw() ); }


#endif //VBAT_H_

