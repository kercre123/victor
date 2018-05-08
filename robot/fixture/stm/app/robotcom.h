#ifndef ROBOTCOM_H
#define ROBOTCOM_H

#include <stdint.h>
#include <limits.h>
#include "cmd.h"

//-----------------------------------------------------------------------------
//                  Helper Comms: Cmd + response parse
//-----------------------------------------------------------------------------

#define DEFAULT_TEMP_ZONE 3

void  helperLcdShow(bool solo, bool invert, char color_rgbw, const char* center_text);
void  helperLcdSetLine(int n, const char* line);
void  helperLcdClear(void);
char* helperGetEmmcdlVersion(int timeout_ms = CMD_DEFAULT_TIMEOUT);
int   helperGetTempC(int zone = DEFAULT_TEMP_ZONE);

//-----------------------------------------------------------------------------
//                  Robot Communications
//-----------------------------------------------------------------------------

//robot sensor index for 'mot' + 'get' cmds
#define RCOM_SENSOR_NONE        0
#define RCOM_SENSOR_BATTERY     1
#define RCOM_SENSOR_CLIFF       2
#define RCOM_SENSOR_MOT_LEFT    3
#define RCOM_SENSOR_MOT_RIGHT   4
#define RCOM_SENSOR_MOT_LIFT    5
#define RCOM_SENSOR_MOT_HEAD    6
#define RCOM_SENSOR_PROX_TOF    7
#define RCOM_SENSOR_BTN_TOUCH   8
#define RCOM_SENSOR_RSSI        9
#define RCOM_SENSOR_RX_PKT      10
#define RCOM_SENSOR_DEBUG_INC   11
const int ccr_sr_cnt[12] = {0,2,4,2,2,2,2,4,2,1,1,4}; //number of CCC sensor fields per line for each type (internal:parsing & error checking)

//FCC test modes
//#define RCOM_FCC_MODE_TX_CARRIER    0
//#define RCOM_FCC_MODE_TX_PACKETS    1
//#define RCOM_FCC_MODE_RX_POWER      2
//#define RCOM_FCC_MODE_RX_PACKETS    3

//rcomEng: index (test selection)
#define RCOM_ENG_IDX_NONE             0
#define RCOM_ENG_IDX_MICS             1 /*XXX not implemented*/
#define RCOM_ENG_IDX_SOUND            2 /*play a tone out the speaker*/

//rcomEng: SOUND params
#define RCOM_ENG_SOUND_DAT0_TONE_BEEP       0
#define RCOM_ENG_SOUND_DAT0_TONE_BELL       1
#define RCOM_ENG_SOUND_DAT1_VOLUME(pctx100) ((pctx100*255)/100)

//data conversion
#define RCOM_BAT_RAW_TO_MV(raw)     (((raw)*2800)>>11)  /*robot_sr_t::bat.raw (adc) to millivolts*/

//debug opts
#define RCOM_PRINT_LEVEL_DEFAULT      RCOM_PRINT_LEVEL_CMD_DAT_RSP
#define RCOM_PRINT_LEVEL_CMD_DAT_RSP  0
#define RCOM_PRINT_LEVEL_CMD_RSP      1
#define RCOM_PRINT_LEVEL_CMD_DAT      2
#define RCOM_PRINT_LEVEL_CMD          3
#define RCOM_PRINT_LEVEL_DAT          4
#define RCOM_PRINT_LEVEL_NONE         5

enum rcom_pwr_st_e {
  RCOM_PWR_ON = 0,
  RCOM_PWR_OFF = 1,
  RCOM_PWR_OFF_IMMEDIATE = 2,
};

typedef struct {
  uint32_t hw_rev;
  uint32_t hw_model;
  uint32_t ein[4];
  uint32_t app_version[4];
} robot_bsv_t;

typedef union {
  int32_t val[4];
  struct { int32_t raw; int32_t temp; } bat; //battery: raw-adc, temperature (2x int16)
  struct { int32_t fL; int32_t fR; int32_t bR; int32_t bL; } cliff; //cliff sensors: front/back L/R (4x uint16)
  struct { int32_t pos; int32_t speed; } enc; //encoder: position, speed (2x int32)
  struct { int32_t rangeMM; int32_t spadCnt; int32_t signalRate; int32_t ambientRate; } prox; //proximity,TOF (4x uint16)
  struct { int32_t touch; int32_t btn; } btn; //touch & button (2x uint16)
  struct { int32_t rssi; } fccRssi; //FCC mode RSSI (int8)
  struct { int32_t pktCnt; } fccRx; //Fcc mode packet rx (int32)
} robot_sr_t;

void          rcomSetTarget(bool spine_nCCC); //select charge contacts or spine for command target (sticky)

uint32_t      rcomEsn(); //read robot ESN (Head)
robot_bsv_t*  rcomBsv(); //read body serial+version info
void          rcomPwr(rcom_pwr_st_e st);
void          rcomLed(uint8_t *leds, int printlvl = RCOM_PRINT_LEVEL_DEFAULT); //leds[12] {led0R,G,B,led1R,G,B...}
robot_sr_t*   rcomMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int printlvl = RCOM_PRINT_LEVEL_DEFAULT);
robot_sr_t*   rcomGet(uint8_t NN, uint8_t sensor, int printlvl = RCOM_PRINT_LEVEL_DEFAULT); //NN = #drops (sr vals). returns &sensor[0] of [NN-1]
int           rcomRlg(uint8_t idx, char *buf, int buf_max_size); //read log 'idx' into buf (NOT null-terminated). return num chars written to buf [e.g. strlen(buf)]
void          rcomEng(uint8_t idx, uint8_t dat0=0, uint8_t dat1=0);
void          rcomSmr(uint8_t idx, uint32_t val);
uint32_t      rcomGmr(uint8_t idx);
//void          rcomFcc(uint8_t mode, uint8_t cn); //RCOM_FCC_MODE_, {0..39}
//void          rcomLfe(uint8_t idx, uint32_t val);


#endif //ROBOTCOM_H

