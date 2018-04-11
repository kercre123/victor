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
#define RCOM_FCC_MODE_TX_CARRIER    0
#define RCOM_FCC_MODE_TX_PACKETS    1
#define RCOM_FCC_MODE_RX_POWER      2
#define RCOM_FCC_MODE_RX_PACKETS    3

//data conversion
#define RCOM_BAT_RAW_TO_MV(raw)     (((raw)*2800)>>11)  /*robot_sr_t::bat.raw (adc) to millivolts*/

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

uint32_t      rcomEsn(); //read robot ESN (Head)
robot_bsv_t*  rcomBsv(); //read body serial+version info
robot_sr_t*   rcomMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int cmd_opts = CMD_OPTS_DEFAULT);
robot_sr_t*   rcomGet(uint8_t NN, uint8_t sensor, int cmd_opts = CMD_OPTS_DEFAULT); //NN = #drops (sr vals). returns &sensor[0] of [NN-1]
void          rcomFcc(uint8_t mode, uint8_t cn); //RCOM_FCC_MODE_, {0..39}
//void          rcomRlg(uint8_t idx);
void          rcomEng(uint8_t idx, uint32_t val);
void          rcomLfe(uint8_t idx, uint32_t val);
void          rcomSmr(uint8_t idx, uint32_t val);
uint32_t      rcomGmr(uint8_t idx);

//-----------------------------------------------------------------------------
//                  Spine HAL
//-----------------------------------------------------------------------------

#include "../../syscon/schema/messages.h"
#include "spine_crc.h"

typedef struct {
  SpineMessageHeader  header;
  union {
    HeadToBody        h2b;
    BodyToHead        b2h;
    MicroBodyToHead   ub2h;
    ContactData       contact;
    VersionInfo       bodyvers;
    AckMessage        ack;
  } payload;
  SpineMessageFooter  footer;
} spinePacket_t;

int             spineSend(uint8_t *payload, PayloadId type);
spinePacket_t*  spineReceive(int timeout_us = 10000);


#endif //ROBOTCOM_H

