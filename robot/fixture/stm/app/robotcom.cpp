#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "console.h"
#include "dut_uart.h"
#include "emrf.h"
#include "fixture.h"
#include "robotcom.h"
#include "timer.h"

//buffer for sensor data
static uint8_t *srbuf = app_global_buffer;
static const int srdatMaxSize = sizeof(robot_sr_t) * 256;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= srdatMaxSize , rcom_sensor_buffer_size_check );

//-----------------------------------------------------------------------------
//                  Helper Comms: Cmd + response parse
//-----------------------------------------------------------------------------

#define HELPER_LCD_CMD_TIMEOUT  150 /*ms*/
#define HELPER_CMD_OPTS_LOGGED  (CMD_OPTS_DEFAULT & ~(CMD_OPTS_EXCEPTION_EN))
#define HELPER_CMD_OPTS_NOLOG   (CMD_OPTS_DEFAULT & ~(CMD_OPTS_EXCEPTION_EN | CMD_OPTS_LOG_ALL))

void helperLcdShow(bool solo, bool invert, char color_rgbw, const char* center_text)
{
  char b[50]; int bz = sizeof(b);
  if( color_rgbw != 'r' && color_rgbw != 'g' && color_rgbw != 'b' && color_rgbw != 'w' )
    color_rgbw = 'w';
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdshow %u %s%c %s", solo, invert?"i":"", color_rgbw, center_text), HELPER_LCD_CMD_TIMEOUT, HELPER_CMD_OPTS_NOLOG);
}

void helperLcdSetLine(int n, const char* line)
{
  char b[50]; int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdset %u %s", n, line), HELPER_LCD_CMD_TIMEOUT, HELPER_CMD_OPTS_NOLOG);
}

void helperLcdClear(void)
{
  helperLcdSetLine(0,""); //clears all lines
  helperLcdShow(0,0,'w',""); //clear center text
}

#define EMMCDL_VER_MAX_LEN  20
static char emmcdl_version[EMMCDL_VER_MAX_LEN+1];
static int emmcdl_read_cnt;

void emmcdl_ver_handler_(char* s) {
  strncpy(emmcdl_version, s, EMMCDL_VER_MAX_LEN);
  emmcdl_version[EMMCDL_VER_MAX_LEN] = '\0';
  emmcdl_read_cnt++;
}

char* helperGetEmmcdlVersion(int timeout_ms)
{
  memset( &emmcdl_version, 0, sizeof(emmcdl_version) );
  emmcdl_read_cnt = 0;
  cmdSend(CMD_IO_HELPER, "get_emmcdl_ver", timeout_ms, HELPER_CMD_OPTS_LOGGED, emmcdl_ver_handler_);
  
  if( cmdStatus() == 0 && emmcdl_read_cnt == 1 && strlen(emmcdl_version) > 0 )
    return emmcdl_version;
  
  //else
  static const char estring[] = "read-error";
  return (char*)estring;
}

enum temp_errs_e {
  TEMP_ERR_HELPER_READ_FAULT  = -1, //returned by helper on file read err, bad zone etc.
  TEMP_ERR_HELPER_NO_RESPONSE = -2, //didn't receive matching zone# response from helper
  TEMP_ERR_HELPER_CMD_ERR     = -3, //helper returned error code to cmd
  TEMP_ERR_LOCAL_PARSE_FAULT  = -4, //internal parsing failed
  TEMP_ERR_LOCAL_PARSE_CNT    = -5, //parser unexpected response count
};

static int tempC, temp_cnt, temp_zone;

void temperature_handler(char* s) {
  temp_cnt++;
  int zone = cmdParseInt32( cmdGetArg(s,0) );
  if( zone == temp_zone ) {
    tempC = cmdParseInt32( cmdGetArg(s,1) );
    if( tempC == INT_MIN ) //parse failed
      tempC = TEMP_ERR_LOCAL_PARSE_FAULT;
  }
}

int helperGetTempC(int zone)
{
  char b[40]; const int bz = sizeof(b);
  snformat(b,bz, (zone >= 0 ? "get_temperature %i" : "get_temperature 0 1 2 3 4 5 6 7 8 9"), zone);
  
  temp_cnt = 0;
  temp_zone = zone >= 0 ? zone : DEFAULT_TEMP_ZONE;
  tempC = TEMP_ERR_HELPER_NO_RESPONSE;
  cmdSend(CMD_IO_HELPER, b, CMD_DEFAULT_TIMEOUT, HELPER_CMD_OPTS_NOLOG, temperature_handler);
  
  if( cmdStatus() != 0 )
    return TEMP_ERR_HELPER_CMD_ERR;
  
  if( (zone < 0 && temp_cnt != 10) || (zone >= 0 && temp_cnt != 1) )
    return TEMP_ERR_LOCAL_PARSE_CNT;
  
  return tempC > 1000 ? tempC/1000 : tempC; //some zones given in mC
}

//-----------------------------------------------------------------------------
//                  Target: Charge Contacts
//-----------------------------------------------------------------------------

#include "cmd.h"

#define CCC_DEBUG 0
#define CCC_CMD_DELAY()     if( CCC_DEBUG > 0 ) { Timer::delayMs(150); }
#define CCC_CMD_OPTS        (CMD_OPTS_DEFAULT | CMD_OPTS_DBG_SYSCON_OVF_ERR)

static struct { //result of most recent command
  error_t err;
  int handler_cnt;
  int sr_cnt;
  union {
    robot_bsv_t bsv;
    uint32_t    esn; 
    uint32_t    emr_val;
  } rsp;
} m_ccc;

//map robotcom's 'printlvl' to cmd option flags
static inline int printlvl2cmdopts( int printlvl ) {
  switch( printlvl) {
    default:
    case RCOM_PRINT_LEVEL_CMD_DAT_RSP:  return CCC_CMD_OPTS & ~(               0 |                0 |                  0 | CMD_OPTS_LOG_OTHER); //break;
    case RCOM_PRINT_LEVEL_CMD_RSP:      return CCC_CMD_OPTS & ~(               0 |                0 | CMD_OPTS_LOG_ASYNC | CMD_OPTS_LOG_OTHER); //break;
    case RCOM_PRINT_LEVEL_CMD:          return CCC_CMD_OPTS & ~(               0 | CMD_OPTS_LOG_RSP | CMD_OPTS_LOG_ASYNC | CMD_OPTS_LOG_OTHER); //break;
    case RCOM_PRINT_LEVEL_NONE:         return CCC_CMD_OPTS & ~(CMD_OPTS_LOG_CMD | CMD_OPTS_LOG_RSP | CMD_OPTS_LOG_ASYNC | CMD_OPTS_LOG_OTHER); //break;
  }
}

//common error check function
static bool ccc_error_check_(int required_handler_cnt)
{
  //Note: m_ccc.err (if any) should be assigned by data handler
  if( m_ccc.err == ERROR_OK && m_ccc.handler_cnt != required_handler_cnt ) //handler didn't run? or didn't find formatting errors
    m_ccc.err = ERROR_TESTPORT_RSP_MISSING_ARGS; //didn't get expected # of responses/lines
  
  if( m_ccc.err != ERROR_OK ) {
    ConsolePrintf("CCC ERROR %i, handler cnt %i/%i\n", m_ccc.err, m_ccc.handler_cnt, required_handler_cnt);
    if( CCC_DEBUG > 0 )
      return 1;
    else
      throw m_ccc.err;
  }
  return 0;
}

void esn_handler_(char* s) {
  m_ccc.rsp.esn = cmdParseHex32( cmdGetArg(s,0) );
  m_ccc.err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_ccc.err;
  m_ccc.handler_cnt++;
}

uint32_t cccEsn()
{
  CCC_CMD_DELAY();
  memset( &m_ccc, 0, sizeof(m_ccc) );
  const char *cmd = "esn 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CCC_CMD_OPTS, esn_handler_);
  
  #if CCC_DEBUG > 0
  ConsolePrintf("esn=%08x\n", m_ccc.rsp.esn );
  #endif
  
  return !ccc_error_check_(1) ? m_ccc.rsp.esn : 0;
}

void bsv_handler_(char* s)
{
  uint32_t *dat = (uint32_t*)&m_ccc.rsp; //bsv is uint32_t array
  const int nwords_max = sizeof(m_ccc.rsp)/sizeof(uint32_t);
  
  if( m_ccc.handler_cnt <= nwords_max-2 ) {
    dat[m_ccc.handler_cnt+0] = cmdParseHex32( cmdGetArg(s,0) );
    m_ccc.err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_ccc.err;
    dat[m_ccc.handler_cnt+1] = cmdParseHex32(cmdGetArg(s,1));
    m_ccc.err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_ccc.err;
  }
  m_ccc.handler_cnt += 2;
}

robot_bsv_t* cccBsv()
{
  CCC_CMD_DELAY();
  memset( &m_ccc, 0, sizeof(m_ccc) );
  const char *cmd = "bsv 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CCC_CMD_OPTS, bsv_handler_);
  
  #if CCC_DEBUG > 0
  robot_bsv_t* bsv = &m_ccc.rsp.bsv;
  ConsolePrintf("bsv hw.rev,model %u %u\n", bsv->hw_rev, bsv->hw_model );
  ConsolePrintf("bsv ein %08x %08x %08x %08x\n", bsv->ein[0], bsv->ein[1], bsv->ein[2], bsv->ein[3] );
  ConsolePrintf("bsv app vers %08x %08x %08x %08x\n", bsv->app_version[0], bsv->app_version[1], bsv->app_version[2], bsv->app_version[3] );
  #endif
  
  return !ccc_error_check_(10) ? &m_ccc.rsp.bsv : NULL;
}

void cccLed(uint8_t *leds, int printlvl) {
  CCC_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  
  //encode led values into ccc cmd format
  uint8_t arg[6];
  for(int i=0; i<12; i++) {
    if( (i&1) == 0 )
      arg[i>>1] = (leds[i] & 0xF0);
    else
      arg[i>>1] |= (leds[i] & 0xF0) >> 4;
  }
  
  #if CCC_DEBUG > 0
  ConsolePrintf("leds"); for(int i=0; i<12; i++) { ConsolePrintf(" %02x", leds[i]); } ConsolePutChar('\n');
  ConsolePrintf("args"); for(int i=0; i<6; i++) { ConsolePrintf(" %02x", arg[i]); } ConsolePutChar('\n');
  #endif
  
  int cmd_opts = printlvl2cmdopts(printlvl);
  snformat(b,bz,"led %02x %02x %02x %02x %02x %02x", arg[0],arg[1],arg[2],arg[3],arg[4],arg[5] );
  cmdSend(CMD_IO_CONTACTS, b, 100, cmd_opts );
}

void sensor_handler_(char* s) {
  robot_sr_t *psr = &((robot_sr_t*)srbuf)[m_ccc.handler_cnt++];
  if( m_ccc.handler_cnt <= 256 ) { //don't overflow buffer
    for(int x=0; x < m_ccc.sr_cnt; x++) {
      psr->val[x] = cmdParseInt32( cmdGetArg(s,x) );
      m_ccc.err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_ccc.err;
    }
  }
}

static robot_sr_t* ccc_MotGet_(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int printlvl, const char* cmd)
{
  CCC_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x %02x", cmd, NN, sensor, (uint8_t)treadL, (uint8_t)treadR, (uint8_t)lift, (uint8_t)head);
  
  memset( &m_ccc, 0, sizeof(m_ccc) );
  memset( srbuf, 0, (1+NN)*sizeof(robot_sr_t) );
  m_ccc.sr_cnt = ccr_sr_cnt[sensor]; //expected number of sensor values per drop/line
  if( !NN || sensor >= sizeof(ccr_sr_cnt) )
    throw ERROR_BAD_ARG;
  
  int cmd_opts = printlvl2cmdopts(printlvl);
  cmdSend(CMD_IO_CONTACTS, b, 500 + (int)NN*10, cmd_opts, sensor_handler_);
  
  #if CCC_DEBUG > 0
  robot_sr_t* psr = (robot_sr_t*)srbuf;
  ConsolePrintf("NN=%u sr vals %i %i %i %i\n", NN, psr[NN-1].val[0], psr[NN-1].val[1], psr[NN-1].val[2], psr[NN-1].val[3] );
  #endif
  
  return !ccc_error_check_(NN) ? (robot_sr_t*)srbuf : NULL;
}

/*void cccFcc(uint8_t mode, uint8_t cn) {
  CCC_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"fcc %02x %02x 00 00 00 00", mode, cn), 500, CCC_CMD_OPTS);
}*/

int cccRlg(uint8_t idx, char *buf, int buf_max_size)
{
  CCC_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  //const int opts = CCC_CMD_OPTS; // & ~(CMD_OPTS_LOG_ASYNC | CMD_OPTS_LOG_OTHER);
        #warning "DEBUG" 
        const int opts = CCC_CMD_OPTS & ~(CMD_OPTS_EXCEPTION_EN);
  cmd_dbuf_t dbuf = { buf, buf_max_size, 0 };
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"rlg %02x 00 00 00 00 00", idx), -1000, opts, 0, (dbuf.p && dbuf.size>0 ? &dbuf : 0) );
  
  //detect and remove the ":LOG n" line inserted by vic-robot parser
  if( dbuf.p && dbuf.wlen > 4 && !strncmp(dbuf.p, ":LOG", 4) ) {
    char *end = strchr(dbuf.p, '\n');
    if( end ) {
      int rmlen = (end - dbuf.p) + 1;
      if( rmlen <= dbuf.wlen ) {
        #if CCC_DEBUG > 0
        ConsolePrintf("REMOVED \"%.*s\"\n", rmlen-1, dbuf.p);
        #endif
        memmove(dbuf.p, end+1, dbuf.wlen-rmlen);
        dbuf.wlen -= rmlen;
      }
    }
  }
  
  return dbuf.wlen;
}

void ccc_IdxVal32_(uint8_t idx, uint32_t val, const char* cmd, void(*handler)(char*), bool print_verbose = true ) {
  CCC_CMD_DELAY();
  memset( &m_ccc, 0, sizeof(m_ccc) );
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x 00", cmd, idx, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF );
  int opts = print_verbose ? CCC_CMD_OPTS : CCC_CMD_OPTS & ~(CMD_OPTS_LOG_CMD | CMD_OPTS_LOG_RSP);
  cmdSend(CMD_IO_CONTACTS, b, 500, opts, handler );
}

void gmr_handler_(char* s) {
  m_ccc.handler_cnt++;
  m_ccc.rsp.emr_val = cmdParseHex32( cmdGetArg(s,0) );
  m_ccc.err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_ccc.err;
}

uint32_t cccGmr(uint8_t idx) {
  ccc_IdxVal32_(idx, 0, "gmr", gmr_handler_, false);
  //ConsolePrintf("emr[%u] = %08x\n", idx, m_ccc.rsp.emr_val);
  return !ccc_error_check_(1) ? m_ccc.rsp.emr_val : 0;
}

//-----------------------------------------------------------------------------
//                  Spine HAL
//-----------------------------------------------------------------------------

#define SPINE_HAL_DEBUG  1

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

static bool payload_type_is_valid_(PayloadId type) {
  switch( type ) {
    case PAYLOAD_DATA_FRAME:
    case PAYLOAD_CONT_DATA:
    case PAYLOAD_MODE_CHANGE:
    case PAYLOAD_VERSION:
    case PAYLOAD_ACK:
    case PAYLOAD_ERASE:
    case PAYLOAD_VALIDATE:
    case PAYLOAD_DFU_PACKET:
    case PAYLOAD_SHUT_DOWN:
    case PAYLOAD_BOOT_FRAME:
      return true;
  }
  return false;
}

static uint16_t payload_size_(PayloadId type, bool h2b) {
  switch( type ) {
    case PAYLOAD_DATA_FRAME:  return h2b ? sizeof(HeadToBody) : sizeof(BodyToHead);
    case PAYLOAD_CONT_DATA:   return sizeof(ContactData);
    case PAYLOAD_MODE_CHANGE: break; //DFU
    case PAYLOAD_VERSION:     return sizeof(VersionInfo);
    case PAYLOAD_ACK:         return sizeof(AckMessage);
    case PAYLOAD_ERASE:       break; //DFU
    case PAYLOAD_VALIDATE:    break; //DFU
    case PAYLOAD_DFU_PACKET:  break; //DFU
    case PAYLOAD_SHUT_DOWN:   break; //XXX
    case PAYLOAD_BOOT_FRAME:  return sizeof(MicroBodyToHead);
    default: break;
  }
  return 0;
}

void halSpineInit(void) {
  #if SPINE_HAL_DEBUG > 0
  ConsolePrintf("hal spine init 3M\n");
  #endif
  DUT_UART::init(3000000);
  Timer::wait(500);
}

typedef struct { int rxDroppedChars; int rxOverflowErrors; int rxFramingErrors; } io_err_t;
static void halSpineGetRxErrs(io_err_t *ioe) {
  ioe->rxDroppedChars   = DUT_UART::getRxDroppedChars();
  ioe->rxOverflowErrors = DUT_UART::getRxOverflowErrors();
  ioe->rxFramingErrors  = DUT_UART::getRxFramingErrors();
}

void halSpineRxFlush(void) {
  while( DUT_UART::getchar() > -1 ) ; //empty rx buf
  io_err_t ioe;
  halSpineGetRxErrs(&ioe); //read to clear errs
}

void halSpineTeardown(void) {
  halSpineRxFlush();
  DUT_UART::deinit();
}

//transmit a spine packet with given payload data
int halSpineSend(uint8_t *payload, PayloadId type)
{
  spinePacket_t txPacket;
  uint16_t payloadlen = payload ? payload_size_(type,1) : 0; //allow 0-length payload
  if( payload && !payloadlen ) //unsupported type
    throw ERROR_BAD_ARG;
  
  txPacket.header.sync_bytes = SYNC_HEAD_TO_BODY;
  txPacket.header.payload_type = type;
  txPacket.header.bytes_to_follow = payloadlen;
  
  if( payload && payloadlen > 0 )
    memcpy( &txPacket.payload, payload, payloadlen );
  
  //place footer at end of payload
  SpineMessageFooter* footer = (SpineMessageFooter*)((uint32_t)&txPacket.payload + payloadlen);
  footer->checksum = calc_crc((const uint8_t*)&txPacket.payload, payloadlen);
  
  #if SPINE_HAL_DEBUG > 0
  ConsolePrintf("spine tx: type %04x len %u checksum %08x\n", txPacket.header.payload_type, txPacket.header.bytes_to_follow, footer->checksum );
  ConsolePrintf(" ->");
  for(int x=0; x < sizeof(SpineMessageHeader) + payloadlen + sizeof(SpineMessageFooter); x++) {
    if( !(x&3) ) ConsolePutChar(' ');
    ConsolePrintf("%02x", ((uint8_t*)&txPacket)[x] );
  } ConsolePrintf("\n");
  #endif
  
  //send to spine uart
  for(int x=0; x < sizeof(SpineMessageHeader) + payloadlen + sizeof(SpineMessageFooter); x++)
    DUT_UART::putchar( ((uint8_t*)&txPacket)[x] );
  
  return sizeof(SpineMessageHeader) + payloadlen + sizeof(SpineMessageFooter); //bytes written
}

spinePacket_t* halSpineReceive(int timeout_us)
{
  int c;
  io_err_t ioe;
  static spinePacket_t rxPacket;
  memset( &rxPacket, 0, sizeof(SpineMessageHeader) );
  
  #if SPINE_HAL_DEBUG > 0
  ConsolePrintf("spine rx: scanning for sync word\n");
  #endif
  
  //scan for packet sync
  uint32_t Tsync = Timer::get();
  while( rxPacket.header.sync_bytes != SYNC_BODY_TO_HEAD )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      throw ERROR_SPINE_PKT_TIMEOUT; //return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
      rxPacket.header.sync_bytes = (rxPacket.header.sync_bytes << 24) | (c & 0xff);
    
    halSpineGetRxErrs(&ioe);
    if( ioe.rxDroppedChars || ioe.rxOverflowErrors )
      throw ERROR_SPINE_RX_ERROR;
  }
  
  #if SPINE_HAL_DEBUG > 0
  ConsolePrintf("spine rx: sync found\n");
  #endif
  
  //get remaining header
  timeout_us = timeout_us > 0 ? 4500 : timeout_us; //alow enough time to rx the packet
  uint8_t *write = (uint8_t*)&rxPacket.header + sizeof(rxPacket.header.sync_bytes);
  while( write < (uint8_t*)&rxPacket.header + sizeof(SpineMessageHeader) )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      throw ERROR_SPINE_PKT_TIMEOUT; //return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
    {
      *write++ = c;
      #if SPINE_HAL_DEBUG > 0
      //DUT_UART::putchar(c);
      #endif
    }
    
    halSpineGetRxErrs(&ioe);
    if( ioe.rxDroppedChars || ioe.rxOverflowErrors )
      throw ERROR_SPINE_RX_ERROR;
  }
  
  #if SPINE_HAL_DEBUG > 0
  ConsolePrintf("spine rx header\n");
  #endif
  
  //get the rest
  //write = (uint8_t*)&rxPacket.payload;
  uint16_t payloadlen = rxPacket.header.bytes_to_follow;
  while( write < (uint8_t*)&rxPacket.payload + payloadlen + sizeof(SpineMessageFooter) )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      throw ERROR_SPINE_PKT_TIMEOUT; //return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
      *write++ = c;
    
    halSpineGetRxErrs(&ioe);
    if( ioe.rxDroppedChars || ioe.rxOverflowErrors )
      throw ERROR_SPINE_RX_ERROR;
  }
  
  //process footer
  SpineMessageFooter* footer = (SpineMessageFooter*)((uint32_t)&rxPacket.payload + payloadlen);
  memcpy( &rxPacket.footer, footer, sizeof(SpineMessageFooter) ); //move to proper pkt locale
  crc_t expected_checksum = calc_crc((const uint8_t*)&rxPacket.payload, payloadlen);
  
  bool type_valid = payload_type_is_valid_(rxPacket.header.payload_type);
  uint16_t expected_len = payload_size_(rxPacket.header.payload_type,0);
  
  #if SPINE_HAL_DEBUG > 0
    ConsolePrintf("spine rx: type %04x len %u checksum %08x\n", rxPacket.header.payload_type, rxPacket.header.bytes_to_follow, rxPacket.footer.checksum );
    if( !type_valid )
      ConsolePrintf("  BAD TYPE: %0x04\n", rxPacket.header.payload_type);
    if( payloadlen != expected_len )
      ConsolePrintf("  BAD LEN: %u/%u\n", payloadlen, expected_len);
    if( expected_checksum != rxPacket.footer.checksum )
      ConsolePrintf("  BAD CHECKSUM: %08x %08x %08x ----\n", expected_checksum, rxPacket.footer.checksum, footer->checksum);
  #endif
  
  if( !type_valid || !expected_len )
    throw ERROR_SPINE_PKT_UNSUPPORTED_TYPE; //return NULL;
  if( expected_checksum != rxPacket.footer.checksum )
    throw ERROR_SPINE_PKT_CHECKSUM; //return NULL;
  
  return &rxPacket;
}

HeadToBody* halSpineH2BFrame(PowerState powerState, int16_t *p4motorPower, uint8_t *p16ledColors)
{
  static uint32_t m_framecounter = 0;
  static HeadToBody h2b;
  
  memset(&h2b, 0, sizeof(h2b));
  h2b.framecounter = ++m_framecounter;
  h2b.powerState = powerState;
  if( p4motorPower != NULL ) memcpy( h2b.motorPower, p4motorPower, 4*sizeof(int16_t) );
  if( p16ledColors != NULL ) memcpy( h2b.ledColors, p16ledColors, 16*sizeof(uint8_t) );
  
  return &h2b;
}

//-----------------------------------------------------------------------------
//                  Target: Spine
//-----------------------------------------------------------------------------

#define SPINE_DEBUG  1

//verify body version structs are equivalent
STATIC_ASSERT( sizeof(robot_bsv_t) == sizeof(VersionInfo), version_struct_size_match_check );

robot_bsv_t* spineBsv_(void)
{
  static robot_bsv_t m_bsv;
  
  //set syscon default state
  ConsolePrintf("spine idle PAYLOAD_DATA_FRAME\n");
  for(int n=0; n<2; n++) {
    halSpineSend((uint8_t*)halSpineH2BFrame(POWER_STATE_ON,0,0), PAYLOAD_DATA_FRAME);
    Timer::delayMs(5);
  }
  
  //Send version request packet (0-payload)
  ConsolePrintf("spine request PAYLOAD_VERSION\n");
  halSpineRxFlush();
  halSpineSend(NULL, PAYLOAD_VERSION);
  
  //Poll for version response
  uint32_t Tstart = Timer::get();
  while( Timer::elapsedUs(Tstart) < 100000 )
  {
    spinePacket_t* pkt = halSpineReceive(10000);
    if( pkt != NULL && pkt->header.payload_type == PAYLOAD_VERSION )
    {
      memcpy( &m_bsv, (VersionInfo*)&pkt->payload, sizeof(robot_bsv_t) );
      
      #if SPINE_DEBUG > 0
      robot_bsv_t* bsv = &m_bsv;
      ConsolePrintf("bsv hw.rev,model %u %u\n", bsv->hw_rev, bsv->hw_model );
      ConsolePrintf("bsv ein %08x %08x %08x %08x\n", bsv->ein[0], bsv->ein[1], bsv->ein[2], bsv->ein[3] );
      ConsolePrintf("bsv app vers %08x %08x %08x %08x\n", bsv->app_version[0], bsv->app_version[1], bsv->app_version[2], bsv->app_version[3] );
      #endif
      
      return &m_bsv;
    }
  }
  
  #if SPINE_DEBUG > 0
  ConsolePrintf("FAILED TO READ BODY VERSION\n");
  #endif
  
  throw ERROR_SPINE_CMD_TIMEOUT; //return NULL;
}

robot_bsv_t* spineBsv(void)
{
  robot_bsv_t* retval = NULL;
  error_t err = ERROR_OK;
  
  halSpineInit();
  try { retval = spineBsv_(); } catch(int e) { err = e; }
  halSpineTeardown();
  
  if( err != ERROR_OK )
    throw err;
  
  return retval;
}

void spinePwr(rcom_pwr_st_e st) {
  //XXX: implement me
  throw ERROR_EMPTY_COMMAND;
}

void spineLed(uint8_t *leds, int printlvl) {
  //XXX: implement me
  throw ERROR_EMPTY_COMMAND;
}

static robot_sr_t* spine_MotGet_(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int printlvl)
{
  //XXX: implement me!
  //return NULL;
  
  ConsolePrintf("spineMot %u %u %u %u %u %u\n", NN, sensor, treadL, treadR, lift, head);
  memset( srbuf, 0, (1+NN)*sizeof(robot_sr_t) );
  if( !NN )
    throw ERROR_BAD_ARG;
  
  return (robot_sr_t*)srbuf;
}

//-----------------------------------------------------------------------------
//                  Robot Communications
//-----------------------------------------------------------------------------

#define RCOM_DEBUG 0

static bool rcom_target_spine_nCCC = 0;
void rcomSetTarget(bool spine_nCCC) {
  rcom_target_spine_nCCC = spine_nCCC;
}

uint32_t rcomEsn() {
  return !rcom_target_spine_nCCC ? cccEsn() : 0; //no head ESN when we are the head...
}

robot_bsv_t* rcomBsv() {
  if( !rcom_target_spine_nCCC )
    return cccBsv();
  else
    return spineBsv();
}

void rcomPwr(rcom_pwr_st_e st) { 
  if( !rcom_target_spine_nCCC )
    ccc_IdxVal32_((int)st,  0,   "pwr", 0);
  else 
    spinePwr(st);
}

void rcomLed(uint8_t *leds, int printlvl) {
  if( !rcom_target_spine_nCCC )
    cccLed(leds, printlvl);
  else 
    spineLed(leds, printlvl);
}

robot_sr_t* rcomMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int printlvl)
{
  if( !rcom_target_spine_nCCC )
    return ccc_MotGet_(NN, sensor, treadL, treadR, lift, head, printlvl, "mot");
  else
    return spine_MotGet_(NN, sensor, treadL, treadR, lift, head, printlvl);
}

robot_sr_t* rcomGet(uint8_t NN, uint8_t sensor, int printlvl)
{
  if( !rcom_target_spine_nCCC )
    return ccc_MotGet_(NN, sensor, 0, 0, 0, 0, printlvl, "get");
  else
    return spine_MotGet_(NN, sensor, 0, 0, 0, 0, printlvl);
}

int  rcomRlg(uint8_t idx, char *buf, int buf_max_size) { return !rcom_target_spine_nCCC ? cccRlg(idx,buf,buf_max_size) : 0; }
void rcomEng(uint8_t idx, uint32_t val) { if( !rcom_target_spine_nCCC ) ccc_IdxVal32_(idx, val, "eng", 0); }
void rcomSmr(uint8_t idx, uint32_t val) { if( !rcom_target_spine_nCCC ) ccc_IdxVal32_(idx, val, "smr", 0, false); }
//void rcomFcc(uint8_t mode, uint8_t cn)  { if( !rcom_target_spine_nCCC ) cccFcc(mode, cn); }
//void rcomLfe(uint8_t idx, uint32_t val) { if( !rcom_target_spine_nCCC ) ccc_IdxVal32_(idx, val, "lfe", 0); }

uint32_t rcomGmr(uint8_t idx) { 
  return !rcom_target_spine_nCCC ? cccGmr(idx) : 0;
}

