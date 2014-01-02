/********************************************************************************************************************
   ------------------------------------------------------------------------------------------------------------------
    myriad2_default.lds.S
   ------------------------------------------------------------------------------------------------------------------
 ********************************************************************************************************************/
 
#define MV_LEONOS_BEGIN   	(__CMX_SLICE12_START)
#define MV_LEONOS_END     	(__CMX_SLICE14_START)
#define MV_LEONRT_BEGIN   	(__CMX_SLICE14_START)
#define MV_LEONRT_END     	(__CMX_END          )

#define MV_SVE0_TEXT_BEGIN  (0x70000000)
#define MV_SVE1_TEXT_BEGIN  (0x70020000)
#define MV_SVE2_TEXT_BEGIN  (0x70040000)
#define MV_SVE3_TEXT_BEGIN  (0x70060000)
#define MV_SVE4_TEXT_BEGIN  (0x70080000)
#define MV_SVE5_TEXT_BEGIN  (0x700A0000)
#define MV_SVE6_TEXT_BEGIN  (0x700C0000)
#define MV_SVE7_TEXT_BEGIN  (0x700E0000)
#define MV_SVE8_TEXT_BEGIN  (0x70100000)
#define MV_SVE9_TEXT_BEGIN  (0x70120000)
#define MV_SVE10_TEXT_BEGIN (0x70140000)
#define MV_SVE11_TEXT_BEGIN (0x70160000)

#define MV_SVE0_DATA_BEGIN  (0x70008000)
#define MV_SVE1_DATA_BEGIN  (0x70028000)
#define MV_SVE2_DATA_BEGIN  (0x70048000)
#define MV_SVE3_DATA_BEGIN  (0x70068000)
#define MV_SVE4_DATA_BEGIN  (0x70088000)
#define MV_SVE5_DATA_BEGIN  (0x700A8000)
#define MV_SVE6_DATA_BEGIN  (0x700C8000)
#define MV_SVE7_DATA_BEGIN  (0x700E8000)
#define MV_SVE8_DATA_BEGIN  (0x70108000)
#define MV_SVE9_DATA_BEGIN  (0x70128000)
#define MV_SVE10_DATA_BEGIN (0x70148000)
#define MV_SVE11_DATA_BEGIN (0x70168000)

#include "shave_cmx.lds.h"
#include "leon.lds.h"
#include "misc.lds.h"

