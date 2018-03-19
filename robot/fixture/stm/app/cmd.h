#ifndef CMD_H
#define CMD_H

#include <stdint.h>
#include <limits.h>

//-----------------------------------------------------------------------------
//                  Master/Send
//-----------------------------------------------------------------------------

//parameterized command/response delimiters (added/removed internally)
#define CMD_PREFIX        ">>"
#define RSP_PREFIX        "<<"
#define ASYNC_PREFIX      ":"
#define LOG_CMD_PREFIX    ">"
#define LOG_RSP_PREFIX    "<"

//io channels - Helper head vs DUT uarts etc.
enum cmd_io_e {
  CMD_IO_SIMULATOR  = 0,
  CMD_IO_HELPER     = 1,
  CMD_IO_CONSOLE    = CMD_IO_HELPER,
  CMD_IO_DUT_UART   = 2,
  CMD_IO_CONTACTS   = 3, //Charge Contact comms
};
typedef enum cmd_io_e cmd_io;

#define CMD_DEFAULT_TIMEOUT           100

//option flags
#define CMD_OPTS_EXCEPTION_EN         0x0001  //enable exceptions
#define CMD_OPTS_LOG_ERRORS           0x0002  //log errors (to console)
#define CMD_OPTS_REQUIRE_STATUS_CODE  0x0004  //missing status code is an error
#define CMD_OPTS_ALLOW_STATUS_ERRS    0x0010  //status!=0 not considered an error
#define CMD_OPTS_DBG_PRINT_ENTRY      0x1000  //print function entry with parsed params
#define CMD_OPTS_DBG_PRINT_RSP_TIME   0x2000  //print elapsed cmd time
#define CMD_OPTS_DEFAULT              (CMD_OPTS_EXCEPTION_EN | CMD_OPTS_LOG_ERRORS | CMD_OPTS_REQUIRE_STATUS_CODE | CMD_OPTS_DBG_PRINT_RSP_TIME)

//Send a command and return response string
//@return response (NULL if timeout)
//e.g. send(IO_DUT, snformat(x,x,"lcdshow %u %s /"Victor DVT/"", solo=0, color="ib"), timeout=100 );
//@param err_lvl: -1 throws exceptions. 0+ prints errors instead, indicates verbosity
char* cmdSend(cmd_io io, const char* scmd, int timeout_ms = CMD_DEFAULT_TIMEOUT, int opts = CMD_OPTS_DEFAULT, void(*async_handler)(char*) = 0 );
int cmdStatus(); //parsed rsp status of most recent cmdSend(). status=1st arg, INT_MIN if !exist or bad format
uint32_t cmdTimeMs(); //time it took for most recent cmdSend() to finish

//during cmdSend() exectuion, callback at the given interval while waiting for response. ONLY for next cmdSend() call; cleared on exit.
void cmdTickCallback(uint32_t interval_ms, void(*tick_handler)(void) );

//-----------------------------------------------------------------------------
//                  Line Parsing
//-----------------------------------------------------------------------------
//Parsing methods for ascii input strings.
//Note: valid strings must guarantee no \r \n chars and one valid \0 terminator

//@return parsed integer value of s. INT_MIN on parse err.
int32_t cmdParseInt32(char *s);

//@return u32 value of input hex string (e.g. 'a235dc01'). 0 on parse error + errno set to -1
uint32_t cmdParseHex32(char* s);

//@return n-th argument (mutable static copy, \0-terminated). NULL if !exist.
//n=0 is command. strings enclosed by "" are treated as a single arg.
char* cmdGetArg(char *s, int n, char* out_buf=0, int buflen=0); //overload out_buf/buflen to provide user buffer to hold the argument

//@return # of args in the input string, including command arg
int cmdNumArgs(char *s);

//DEBUG: run some parsing tests
void cmdDbgParseTestbench(void);

//-----------------------------------------------------------------------------
//                  Robot (Charge Contacts)
//-----------------------------------------------------------------------------
//cmdSend() to robot over charge contacts - parse reply into data struct.

//sensor index for 'mot' + 'get' cmds
#define CCC_SENSOR_NONE       0
#define CCC_SENSOR_BATTERY    1
#define CCC_SENSOR_CLIFF      2
#define CCC_SENSOR_MOT_LEFT   3
#define CCC_SENSOR_MOT_RIGHT  4
#define CCC_SENSOR_MOT_LIFT   5
#define CCC_SENSOR_MOT_HEAD   6
#define CCC_SENSOR_PROX_TOF   7
#define CCC_SENSOR_BTN_TOUCH  8
#define CCC_SENSOR_RSSI       9
#define CCC_SENSOR_RX_PKT     10
const int ccr_sr_cnt[11] = {0,2,4,2,2,2,2,4,2,1,1}; //number of sensor fields for each type

//FCC test modes
#define CCC_FCC_MODE_TX_CARRIER   0
#define CCC_FCC_MODE_TX_PACKETS   1
#define CCC_FCC_MODE_RX_POWER     2
#define CCC_FCC_MODE_RX_PACKETS   3

typedef struct {
  uint32_t esn;
} ccr_esn_t;

typedef struct {
  uint32_t hw_rev;
  uint32_t hw_model;
  uint32_t ein[4];
  uint32_t app_version[4];
} ccr_bsv_t;

typedef union {
  int32_t val[4];
  struct { int32_t raw; int32_t temp; } bat; //battery: raw-adc, temperature (2x int16)
  struct { int32_t fL; int32_t fR; int32_t bR; int32_t bL; } cliff; //cliff sensors: front/back L/R (4x uint16)
  struct { int32_t pos; int32_t speed; } enc; //encoder: position, speed (2x int32)
  struct { int32_t rangeMM; int32_t spadCnt; int32_t signalRate; int32_t ambientRate; } prox; //proximity,TOF (4x uint16)
  struct { int32_t touch; int32_t btn; } btn; //touch & button (2x uint16)
  struct { int32_t rssi; } fccRssi; //FCC mode RSSI (int8)
  struct { int32_t pktCnt; } fccRx; //Fcc mode packet rx (int32)
} ccr_sr_t;

ccr_esn_t* cmdRobotEsn(); //read robot (head) ESN
ccr_bsv_t* cmdRobotBsv(); //read body serial+version info
ccr_sr_t*  cmdRobotMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head);
ccr_sr_t*  cmdRobotGet(uint8_t NN, uint8_t sensor); //NN = #drops (sr vals). returns &sensor[0] of [NN-1]
void       cmdRobotFcc(uint8_t mode, uint8_t cn); //CCC_FCC_MODE_, {0..39}
//void       cmdRobotRlg(uint8_t idx);
void       cmdRobotEng(uint8_t idx, uint32_t val);
void       cmdRobotLfe(uint8_t idx, uint32_t val);
void       cmdRobotSmr(uint8_t idx, uint32_t val);
uint32_t   cmdRobotGmr(uint8_t idx);

//-----------------------------------------------------------------------------
//                  Additional Cmd + response parsing
//-----------------------------------------------------------------------------

#define DEFAULT_TEMP_ZONE 3
char* cmdGetEmmcdlVersion(int timeout_ms = CMD_DEFAULT_TIMEOUT);
int   cmdGetHelperTempC(int zone = DEFAULT_TEMP_ZONE);

#endif //CMD_H

