#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
//#include "board.h"
#include "cmd.h"
#include "console.h"
//#include "contacts.h"
#include "dut_uart.h"
#include "emrf.h"
#include "fixture.h"
#include "robotcom.h"
#include "timer.h"

//-----------------------------------------------------------------------------
//                  Helper Comms: Cmd + response parse
//-----------------------------------------------------------------------------

#define HELPER_LCD_CMD_TIMEOUT 150 /*ms*/

void helperLcdShow(bool solo, bool invert, char color_rgbw, const char* center_text)
{
  char b[50]; int bz = sizeof(b);
  if( color_rgbw != 'r' && color_rgbw != 'g' && color_rgbw != 'b' && color_rgbw != 'w' )
    color_rgbw = 'w';
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdshow %u %s%c %s", solo, invert?"i":"", color_rgbw, center_text), HELPER_LCD_CMD_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
}

void helperLcdSetLine(int n, const char* line)
{
  char b[50]; int bz = sizeof(b);
  cmdSend(CMD_IO_HELPER, snformat(b,bz,"lcdset %u %s", n, line), HELPER_LCD_CMD_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN);
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
  cmdSend(CMD_IO_HELPER, "get_emmcdl_ver", timeout_ms, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN, emmcdl_ver_handler_);
  
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
  cmdSend(CMD_IO_HELPER, b, CMD_DEFAULT_TIMEOUT, CMD_OPTS_DEFAULT & ~CMD_OPTS_EXCEPTION_EN, temperature_handler);
  
  if( cmdStatus() != 0 )
    return TEMP_ERR_HELPER_CMD_ERR;
  
  if( (zone < 0 && temp_cnt != 10) || (zone >= 0 && temp_cnt != 1) )
    return TEMP_ERR_LOCAL_PARSE_CNT;
  
  return tempC > 1000 ? tempC/1000 : tempC; //some zones given in mC
}

//-----------------------------------------------------------------------------
//                  Robot Communications
//-----------------------------------------------------------------------------

#define RCOM_DEBUG 0
#define RCOM_CMD_DELAY()     if( RCOM_DEBUG > 0 ) { Timer::delayMs(150); }

//buffer for sensor data
static uint8_t *srbuf = app_global_buffer;
static const int srdatMaxSize = sizeof(robot_sr_t) * 256;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= srdatMaxSize , rcom_sensor_buffer_size_check );

static struct { //result of most recent command
  error_t ccr_err;
  int handler_cnt;
  int sr_cnt;
  union {
    robot_bsv_t bsv;
    uint32_t    esn; 
    uint32_t    emr_val;
  } rsp;
} m_dat;

//common error check function
static bool rcom_error_check_(int required_handler_cnt)
{
  //Note: m_dat.ccr_err (if any) should be assigned by data handler
  if( m_dat.ccr_err == ERROR_OK && m_dat.handler_cnt != required_handler_cnt ) //handler didn't run? or didn't find formatting errors
    m_dat.ccr_err = ERROR_TESTPORT_RSP_MISSING_ARGS; //didn't get expected # of responses/lines
  
  //DEBUG XXX: throw error on mid-packet line sync
  //if( m_dat.ccr_err == ERROR_OK && m_ioerr.linesyncErrors > 0 )
  //  m_dat.ccr_err = ERROR_TESTPORT_RX_ERROR;
  
  if( m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("ERROR %i, handler cnt %i/%i\n", m_dat.ccr_err, m_dat.handler_cnt, required_handler_cnt);
    if( RCOM_DEBUG > 0 )
      return 1;
    else
      throw m_dat.ccr_err;
  }
  return 0;
}

void esn_handler_(char* s) {
  m_dat.rsp.esn = cmdParseHex32( cmdGetArg(s,0) );
  m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
  m_dat.handler_cnt++;
}

uint32_t rcomEsn()
{
  RCOM_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  const char *cmd = "esn 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CMD_OPTS_DEFAULT, esn_handler_);
  
  #if RCOM_DEBUG > 0
  ConsolePrintf("esn=%08x\n", m_dat.rsp.esn.esn );
  #endif
  
  return !rcom_error_check_(1) ? m_dat.rsp.esn : 0;
}

void bsv_handler_(char* s)
{
  uint32_t *dat = (uint32_t*)&m_dat.rsp; //bsv is uint32_t array
  const int nwords_max = sizeof(m_dat.rsp)/sizeof(uint32_t);
  
  if( m_dat.handler_cnt <= nwords_max-2 ) {
    dat[m_dat.handler_cnt+0] = cmdParseHex32( cmdGetArg(s,0) );
    m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
    dat[m_dat.handler_cnt+1] = cmdParseHex32(cmdGetArg(s,1));
    m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
  }
  m_dat.handler_cnt += 2;
}

robot_bsv_t* rcomBsv()
{
  RCOM_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  const char *cmd = "bsv 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CMD_OPTS_DEFAULT, bsv_handler_);
  
  #if RCOM_DEBUG > 0
  ConsolePrintf("bsv hw.rev,model %u %u\n", m_dat.rsp.bsv.hw_rev, m_dat.rsp.bsv.hw_model );
  ConsolePrintf("bsv ein %08x %08x %08x %08x\n", m_dat.rsp.bsv.ein[0], m_dat.rsp.bsv.ein[1], m_dat.rsp.bsv.ein[2], m_dat.rsp.bsv.ein[3] );
  ConsolePrintf("bsv app vers %08x %08x %08x %08x\n", m_dat.rsp.bsv.app_version[0], m_dat.rsp.bsv.app_version[1], m_dat.rsp.bsv.app_version[2], m_dat.rsp.bsv.app_version[3] );
  #endif
  
  rcom_error_check_(10);
  return &m_dat.rsp.bsv;
}

void sensor_handler_(char* s) {
  robot_sr_t *psr = &((robot_sr_t*)srbuf)[m_dat.handler_cnt++];
  if( m_dat.handler_cnt <= 256 ) { //don't overflow buffer
    for(int x=0; x < m_dat.sr_cnt; x++) {
      psr->val[x] = cmdParseInt32( cmdGetArg(s,x) );
      m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_dat.ccr_err;
    }
  }
}

static robot_sr_t* rcom_MotGet_(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, const char* cmd, int cmd_opts)
{
  RCOM_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x %02x", cmd, NN, sensor, (uint8_t)treadL, (uint8_t)treadR, (uint8_t)lift, (uint8_t)head);
  
  memset( &m_dat, 0, sizeof(m_dat) );
  memset( srbuf, 0, (1+NN)*sizeof(robot_sr_t) );
  m_dat.sr_cnt = sensor >= sizeof(ccr_sr_cnt) ? 0 : ccr_sr_cnt[sensor]; //expected number of sensor values per drop/line
  if( !NN )
    throw ERROR_BAD_ARG;
  
  cmdSend(CMD_IO_CONTACTS, b, 500 + (int)NN*10, cmd_opts, sensor_handler_);
  
  #if RCOM_DEBUG > 0
  robot_sr_t* psr = (robot_sr_t*)srbuf;
  ConsolePrintf("NN=%u sr vals %i %i %i %i\n", NN, psr[NN-1].val[0], psr[NN-1].val[1], psr[NN-1].val[2], psr[NN-1].val[3] );
  #endif
  
  rcom_error_check_(NN);
  return (robot_sr_t*)srbuf;
}
robot_sr_t* rcomMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, int cmd_opts) {
  return rcom_MotGet_(NN, sensor, treadL, treadR, lift, head, "mot", cmd_opts);
}
robot_sr_t* rcomGet(uint8_t NN, uint8_t sensor, int cmd_opts) {
  return rcom_MotGet_(NN, sensor, 0, 0, 0, 0, "get", cmd_opts);
}

void rcomFcc(uint8_t mode, uint8_t cn) {
  RCOM_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"fcc %02x %02x 00 00 00 00", mode, cn), 500);
}

void rcomRlg(uint8_t idx) {
  RCOM_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  ConsolePrintf("XXX: rlg not implemented\n");
  throw ERROR_EMPTY_COMMAND;
}

void rcom_IdxVal32_(uint8_t idx, uint32_t val, const char* cmd, void(*handler)(char*), bool print_verbose = true ) {
  RCOM_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x 00", cmd, idx, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF );
  int opts = print_verbose ? CMD_OPTS_DEFAULT : CMD_OPTS_DEFAULT & ~(CMD_OPTS_LOG_CMD | CMD_OPTS_LOG_RSP);
  cmdSend(CMD_IO_CONTACTS, b, 500, opts, handler );
}
void rcomEng(uint8_t idx, uint32_t val) { rcom_IdxVal32_(idx, val, "eng", 0); }
void rcomLfe(uint8_t idx, uint32_t val) { rcom_IdxVal32_(idx, val, "lfe", 0); }
void rcomSmr(uint8_t idx, uint32_t val) { rcom_IdxVal32_(idx, val, "smr", 0, false); }

void gmr_handler_(char* s) {
  m_dat.handler_cnt++;
  m_dat.rsp.emr_val = cmdParseHex32( cmdGetArg(s,0) );
  m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_dat.ccr_err;
}

uint32_t rcomGmr(uint8_t idx)
{
  rcom_IdxVal32_(idx, 0, "gmr", gmr_handler_, false);
  //ConsolePrintf("emr[%u] = %08x\n", idx, m_dat.rsp.emr_val);
  
  rcom_error_check_(1);
  return m_dat.rsp.emr_val;
}

//-----------------------------------------------------------------------------
//                  Spine HAL
//-----------------------------------------------------------------------------

#define RCOM_SPINE_DEBUG  1

const int SPINE_BAUD = 3000000;

static bool valid_payload_type_(PayloadId type) {
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
    case PAYLOAD_MODE_CHANGE: break;
    case PAYLOAD_VERSION:     return sizeof(VersionInfo);
    case PAYLOAD_ACK:         return sizeof(AckMessage);
    case PAYLOAD_ERASE:       break;
    case PAYLOAD_VALIDATE:    break;
    case PAYLOAD_DFU_PACKET:  break;
    case PAYLOAD_SHUT_DOWN:   break;
    case PAYLOAD_BOOT_FRAME:  return sizeof(MicroBodyToHead);
    default: break;
  }
  throw ERROR_BAD_ARG; //return 0;
}

//transmit a spine packet with given payload data
int spineSend(uint8_t *payload, PayloadId type)
{
  DUT_UART::init(SPINE_BAUD); //protected from re-init
  
  spinePacket_t txPacket;
  uint16_t payloadlen = payload_size_(type,1);
  
  txPacket.header.sync_bytes = SYNC_HEAD_TO_BODY;
  txPacket.header.payload_type = type;
  txPacket.header.bytes_to_follow = payloadlen;
  
  memcpy( &txPacket.payload, payload, payloadlen );
  
  //place footer at end of payload
  SpineMessageFooter* footer = (SpineMessageFooter*)((uint32_t)&txPacket.payload + payloadlen);
  uint32_t sum = 0;
  for(int x=0; x<payloadlen; x++)
    sum += payload[x];
  footer->checksum = sum;
  
  #if RCOM_SPINE_DEBUG > 0
  ConsolePrintf("spine tx: type %04x len %u checksum %08x\n", txPacket.header.payload_type, txPacket.header.bytes_to_follow, footer->checksum );
  #endif
  
  //send to spine uart
  for(int x=0; x < sizeof(SpineMessageHeader) + payloadlen + sizeof(SpineMessageFooter); x++)
    DUT_UART::putchar( ((uint8_t*)&txPacket)[x] );
  
  return sizeof(SpineMessageHeader) + payloadlen + sizeof(SpineMessageFooter); //bytes written
}

static spinePacket_t rxPacket;
spinePacket_t* spineReceive(int timeout_us)
{
  DUT_UART::init(SPINE_BAUD); //protected from re-init
  
  int c;
  memset( &rxPacket, 0, sizeof(SpineMessageHeader) );
  
  //scan for packet sync
  uint32_t Tsync = Timer::get();
  while( rxPacket.header.sync_bytes != SYNC_BODY_TO_HEAD )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
      rxPacket.header.sync_bytes = (rxPacket.header.sync_bytes << 24) | (c & 0xff);
  }
  
  //get remaining header
  timeout_us = timeout_us > 0 ? 4500 : timeout_us; //alow enough time to rx the packet
  uint8_t *write = (uint8_t*)&rxPacket.header + sizeof(rxPacket.header.sync_bytes);
  while( write < (uint8_t*)&rxPacket.header + sizeof(SpineMessageHeader) )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
      *write++ = c;
  }
  
  //get the rest
  //write = (uint8_t*)&rxPacket.payload;
  uint16_t payloadlen = rxPacket.header.bytes_to_follow;
  while( write < (uint8_t*)&rxPacket.payload + payloadlen + sizeof(SpineMessageFooter) )
  {
    if( timeout_us > 0 && Timer::elapsedUs(Tsync) > timeout_us )
      return NULL;
    if( (c = DUT_UART::getchar(0)) > -1 )
      *write++ = c;
  }
  
  //move footer and error check
  SpineMessageFooter* footer = (SpineMessageFooter*)((uint32_t)&rxPacket.payload + payloadlen);
  memcpy( &rxPacket.footer, footer, sizeof(SpineMessageFooter) );
  uint32_t sum = 0;
  for(int x=0; x<payloadlen; x++)
    sum += ((uint8_t*)&rxPacket.payload)[x];
  
  #if RCOM_SPINE_DEBUG > 0
    uint16_t expected_len = 0;
    try{ expected_len = payload_size_(rxPacket.header.payload_type,0); } catch(int e){}
    ConsolePrintf("spine rx: type %04x len %u checksum %08x\n", rxPacket.header.payload_type, rxPacket.header.bytes_to_follow, rxPacket.footer.checksum );
    if( !valid_payload_type_(rxPacket.header.payload_type) )
      ConsolePrintf("  BAD TYPE: %0x04\n", rxPacket.header.payload_type);
    if( payloadlen != expected_len )
      ConsolePrintf("  BAD LEN: %u/%u\n", payloadlen, expected_len);
    if( sum != rxPacket.footer.checksum )
      ConsolePrintf("  BAD CHECKSUM: %08x %08x %08x ----\n", sum, rxPacket.footer.checksum, footer->checksum);
  #endif
  
  if( sum != rxPacket.footer.checksum )
    return NULL;
  
  return &rxPacket;
}

