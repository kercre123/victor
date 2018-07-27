#ifndef EMRF_H
#define EMRF_H
//Description: definitions for 'fixture' region of the EMR (birth certificate)

#include <stdint.h>

#ifdef __cplusplus

#include "../../../include/anki/cozmo/shared/factory/emr.h" //common emr structure

//#define EMR_FIELD_OFS(fieldname)  offsetof(Anki::Cozmo::Factory::EMR::Fields, fieldname)/sizeof(uint32_t)
#define EMR_FIELD_OFS(fieldname) ( \
  ((uint32_t)&((Anki::Cozmo::Factory::EMR*)0)->fields.fieldname) / sizeof(uint32_t) \
  - ((uint32_t)&((Anki::Cozmo::Factory::EMR*)0)->data[0]) / sizeof(uint32_t)  \
)

typedef struct {
  int8_t power;
  uint8_t reserved[3];
  struct { int16_t speed; int16_t travel; } fwd;
  struct { int16_t speed; int16_t travel; } rev;
} emr_tread_dat_t;
typedef char static_assertion_emr_tread_dat_size_check[(sizeof(emr_tread_dat_t) == 12) ? 1 : -1];

typedef struct {
  uint8_t NN;
  int8_t power;
  uint8_t reserved[2];
  struct { int16_t speed; int16_t travel; } up;
  struct { int16_t speed; int16_t travel; } dn;
} emr_range_dat_t;
typedef char static_assertion_emr_range_dat_size_check[(sizeof(emr_range_dat_t) == 12) ? 1 : -1];

//map EMR::fixture[i] field definitions
#define EMRF_PACKOUT_CNT                  0   //# of packouts (e.g. 1 + factory rework cycles)
#define EMRF_PACKOUT_VBAT_MV              1   //battery level measured from Packout
#define EMRF_ROBOT3_VBAT_MV               2   //battery level measured from Robot3
#define EMRF_ROBOT3_TREAD_L_LO            3   //4,5   tread test data, left , low speed
#define EMRF_ROBOT3_TREAD_R_LO            6   //7,8   tread test data, right, low speed
#define EMRF_ROBOT3_TREAD_L_HI            9   //10,11 tread test data, left , high speed
#define EMRF_ROBOT3_TREAD_R_HI            12  //13,14 tread test data, right, high speed
#define EMRF_ROBOT3_RANGE_LIFT_LO         15  //16,17 range test data, lift , low power
#define EMRF_ROBOT3_RANGE_HEAD_LO         18  //19,20 range test data, head , low power
#define EMRF_ROBOT3_RANGE_LIFT_HI         21  //22,23 range test data, lift , high power
#define EMRF_ROBOT3_RANGE_HEAD_HI         24  //25,26 range test data, head , high power


#endif //__cplusplus
#endif //EMRF_H

