#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "app.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "contacts.h"
#include "dut_uart.h"
#include "fixture.h"
//#include "portable.h"
#include "timer.h"
#include "uart.h"

#define CMD_DEBUG 0

static int m_status; //status code parsed from the most recent command
int cmdStatus() {
  return m_status;
}

static uint32_t m_time_ms; //runtime for most recent command
uint32_t cmdTimeMs() {
  return m_time_ms;
}

//-----------------------------------------------------------------------------
//          Master/Send
//-----------------------------------------------------------------------------

static int rsp_len = 0;
static void flush_(cmd_io io)
{
  rsp_len = 0;
  switch(io)
  {
    case CMD_IO_DUT_UART:
      while( DUT_UART::getchar() > -1 );
      break;
    case CMD_IO_CONSOLE:
      ConsoleFlushLine(); //flush old data in the line buffer
      while( ConsoleGetCharNoWait() > -1 ); //flush unread dma buf
      break;
    case CMD_IO_CONTACTS:
      Contacts::flushRx(); //empty line buffer
      while( Contacts::getchar() > -1 );
      break;
    default:
    case CMD_IO_SIMULATOR:
      break;
  }
}

static char* getline_(cmd_io io, int timeout_us)
{
  static char rsp_buf[100];
  char* rsp;
  int out_len;
  
  switch(io)
  {
    case CMD_IO_DUT_UART:
      rsp = DUT_UART::getline( &rsp_buf[rsp_len], sizeof(rsp_buf)-rsp_len, timeout_us, &out_len );
      rsp_len += out_len;
      if( rsp ) {
        rsp = rsp_buf; //point to start of buffer (rx in multiple chunks)
        rsp_len = 0;
      }
      break;
    case CMD_IO_CONSOLE:
      rsp = ConsoleGetLine(timeout_us);
      break;
    case CMD_IO_CONTACTS:
      rsp = Contacts::getline(timeout_us);
      break;
    default:
    case CMD_IO_SIMULATOR:
      rsp = NULL;
      break;
  }
  
  return rsp;
}

static void write__(cmd_io io, const char* s)
{
  switch(io)
  {
    case CMD_IO_DUT_UART:
      DUT_UART::write(s);
      break;
    case CMD_IO_CONSOLE:
      ConsoleWrite((char*)s);
      break;
    case CMD_IO_CONTACTS:
      Contacts::write(s);
      break;
    default:
    case CMD_IO_SIMULATOR:
      //uh...simulate! (waving hands magically)
      break;
  }
}

static void write_(cmd_io io, const char* s1, const char* s2=NULL, const char* s3=NULL);
static void write_(cmd_io io, const char* s1, const char* s2, const char* s3) {
  if(s1) write__(io, s1);
  if(s2) write__(io, s2);
  if(s3) write__(io, s3);
}

#define ERROR_HANDLE(str,err_num)                       \
  if( opts & CMD_OPTS_LOG_ERRORS ) {                    \
    write_(CMD_IO_CONSOLE, LOG_RSP_PREFIX, (str));      \
  }                                                     \
  if( opts & CMD_OPTS_EXCEPTION_EN ) {                  \
    throw (err_num);                                    \
  }

//double-buffered tick handling
static uint32_t next_tick_interval_ms = 0, active_tick_interval_ms = 0;
static void(*next_tick_handler)(void) = NULL;
static void(*active_tick_handler)(void) = NULL;

void cmdTickCallback(uint32_t interval_ms, void(*tick_handler)(void) ) {
  next_tick_interval_ms = interval_ms && tick_handler ? interval_ms  : 0;
  next_tick_handler     = interval_ms && tick_handler ? tick_handler : NULL;
}

char* cmdSend(cmd_io io, const char* scmd, int timeout_ms, int opts, void(*async_handler)(char*) )
{
  char b[40]; int bz = sizeof(b); //str buffer
  
  //check if we should insert newlines?
  bool newline = scmd[strlen(scmd)-1] != '\n';
  
  if( opts & CMD_OPTS_DBG_PRINT_ENTRY )
    ConsolePrintf("cmdSend(%u, \"%s\"%s, ms=%i)\n", io, scmd, (newline ? "\\n" : ""), timeout_ms);
  
  //double buffer the tick handler
  active_tick_interval_ms = next_tick_interval_ms;
  active_tick_handler = next_tick_handler;
  next_tick_interval_ms = 0, next_tick_handler = NULL; //single-use
  
  //send command out the selected io channel (append prefix)
  flush_(io); //flush rx buffers first for correct response detection
  if( io != CMD_IO_CONSOLE ) //echo to log BEFORE to DUT write or we'll miss response chars
    write_(CMD_IO_CONSOLE, LOG_CMD_PREFIX, scmd, (newline ? "\n" : NULL) );
  write_(io, CMD_PREFIX, scmd, (newline ? "\n" : NULL) );

  //Get response
  m_status = INT_MIN;
  uint32_t Tstart = Timer::get(), Ttick = Timer::get();
  while( Timer::elapsedUs(Tstart) < timeout_ms*1000 )
  {
    char *rsp = getline_(io, 1000); //timeout_ms*1000 - Timer::elapsedUs(Tstart) );
    if( rsp != NULL )
    {
      int rspLen = strlen(rsp);
      
      //find and validate the response
      if( rspLen > sizeof(RSP_PREFIX)-1 && !strncmp(rsp,RSP_PREFIX,sizeof(RSP_PREFIX)-1) ) //response prefix detected
      {
        m_time_ms = Timer::elapsedUs(Tstart)/1000; //time to response rx
        
        rsp += sizeof(RSP_PREFIX)-1; //'strip' prefix
        
        //echo to log (optionally append debug stats)
        write_(CMD_IO_CONSOLE, io == CMD_IO_CONSOLE ? RSP_PREFIX : LOG_RSP_PREFIX, rsp); //differentiate prefix depending on the sender
        if( opts & CMD_OPTS_DBG_PRINT_RSP_TIME )
          write_(CMD_IO_CONSOLE, snformat(b,bz," [%ums]", m_time_ms));
        write_(CMD_IO_CONSOLE, "\n");
        
        //make sure response matches our command word
        int nargs = cmdNumArgs(rsp);
        if( nargs < 1 || strcmp( cmdGetArg((char*)scmd,0,b,bz), cmdGetArg(rsp,0)) != 0 ) {
          ERROR_HANDLE("MISMATCH\n", ERROR_TESTPORT_RSP_MISMATCH);
          return NULL;
        }
        
        //enforce valid status code as first rsp arg
        if( nargs < 2 && opts & CMD_OPTS_REQUIRE_STATUS_CODE ) {
          //validate numeric?
          ERROR_HANDLE("MISSING ARGS\n", ERROR_TESTPORT_RSP_MISSING_ARGS);
          return NULL;
        }

        //try parse the 2nd arg as status code
        if( nargs >= 2 )
        {
          int val = cmdParseInt32( cmdGetArg(rsp,1) );
          if( val != INT_MIN )
            m_status = val; //record parsed value
        }
        
        //check for status code errors (if not allowed)
        if( nargs >= 2 && m_status != 0 && !(opts & CMD_OPTS_ALLOW_STATUS_ERRS) ) {
          ERROR_HANDLE("CMD FAILED\n", ERROR_TESTPORT_CMD_FAILED);
          return NULL;
        }
        
        //passed all checks
        return rsp;
      }
      else
      {
        if( io != CMD_IO_CONSOLE ) //echo to log, unchanged
          write_(CMD_IO_CONSOLE, rsp, "\n");
        
        //async data while waiting for cmd completion
        if( async_handler != NULL ) {
          if( !strncmp(rsp,ASYNC_PREFIX,sizeof(ASYNC_PREFIX)-1) && rspLen > sizeof(ASYNC_PREFIX)-1 )
            async_handler(rsp + sizeof(ASYNC_PREFIX)-1);
        }
      }
    }
    
    //tick callback
    if( active_tick_interval_ms > 0 && Timer::elapsedUs(Ttick) >= 1000*active_tick_interval_ms ) {
      Ttick = Timer::get();
      if( active_tick_handler != NULL )
        active_tick_handler();
    }
  }
  
  m_time_ms = timeout_ms;
  ERROR_HANDLE(snformat(b,bz,"TIMEOUT %ums\n", timeout_ms), ERROR_TESTPORT_CMD_TIMEOUT);
  return NULL;
}

//-----------------------------------------------------------------------------
//                  Slave/Parsing
//-----------------------------------------------------------------------------
//Note: valid command strings must guarantee no \r \n chars and one valid \0 terminator

static inline bool isWhitespace_(char c) {
  return c <= ' ' || c > '~'; //space or non-printable char
}

//seek to start of next argument.
static char* nextArg_(char* s)
{
  //started on a quoted arg. seek to end of quotes
  bool inQuotes = *s == '"';
  while( inQuotes ) {
    if( *++s == NULL )
      return NULL;
    if( *s == '"' )
      break;
  }
  
  //started on normal arg (or closing quote from above). seek next whitespace
  while( !isWhitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  //finally, seek start of next arg
  while( isWhitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  return s; //now pointing to first char of next arg (maybe an opening ")
}

int cmdParseInt32(char *s)
{
  if(s)
  {
    errno = 0;
    char *endptr;
    long val = strtol(s, &endptr, 10); //enforce base10

    //check conversion errs, limit numerical range, and verify entire arg was parsed (stops at whitespace, invalid chars...)
    if( errno == 0 && val <= INT_MAX && val >= INT_MIN+1 && endptr > s && *endptr == '\0' )
      return val;
  }
  return INT_MIN; //parse error
}

uint32_t cmdParseHex32(char* s)
{
  if(s)
  {
    int len = strlen(s);
    if( len <= 10 )
    {
      char buf[11];
      memcpy(buf, s, len);
      buf[len] = '\0';
      
      errno = 0;
      uint32_t low = strtol(buf+len-4,0,16); //last 4 chars
      buf[len-4] = '\0'; //remove low
      uint32_t high = strtol(buf,0,16);
      
      return (high << 16) | low;
    }
  }
  errno = -1;
  return 0; //parse error
}

char* cmdGetArg(char *s, int n, char* out_buf, int buflen)
{
  const int bufsize_ = 51;
  static char buf_[bufsize_+1];
  
  int bufsize = out_buf && buflen > 0 ? buflen-1 : bufsize_;
  char* buf = out_buf && buflen > 0 ? out_buf : buf_;
  
  if( isWhitespace_(*s) ) //need initial seek to start from 0th arg
    n++;
  while(n--) { //seek to nth arg
    if( (s = nextArg_(s)) == NULL )
      return NULL; //there is no spoon
  }
  
  //find arg length
  int len = 0;
  bool inQuotes = *s == '"';
  if( inQuotes ) {
    char* temp = ++s;
    while(1) {
      if(*temp == '"' || *temp == NULL)
        break;
      len++; temp++;
    }
  } 
  else { //normal arg
    char* temp = s;
    do { 
      len++;
    } while(*++temp != NULL && !isWhitespace_(*temp) );
  }
  
  //copy to our buffer so we can modify (null terminate)
  len = len > bufsize ? bufsize : len; //limit to buffer size
  memcpy(buf, s, len );
  buf[len] = '\0';
  return buf;
}

int cmdNumArgs(char *s)
{
  if( s == NULL )
    return 0;
  
  int n = isWhitespace_(*s) ? 0 : 1;
  while( (s = nextArg_(s)) != NULL )
    n++;
  return n;
}

//-----------------------------------------------------------------------------
//          Debug
//-----------------------------------------------------------------------------

//uncomment to enable testbench
//#define DBG_PRINTF(...)   ConsolePrintf(__VA_ARGS__)

void cmdDbgParseTestbench(void)
{
#ifdef DBG_PRINTF
  const char* samples[] = {
    "cmd arg1 arg2 arg3",
    "  cmd   arg 1 arg3  ",
    ">> cmd \"a r g 1\" arg2",
    "cmd \"non terminated",
    " cmd arg1   arg2  \"arg 3\"discarded arg4",
    "   \"",
    "     ",
    "comm\"and x ",
  };
  
  DBG_PRINTF("\ncmd parser testbench:\n");
  
  for(int x=0; x<sizeof(samples)/sizeof(char*); x++)
  {
    int numargs = cmdNumArgs((char*)samples[x]);
    DBG_PRINTF("in: '%s'  cmdNumArgs()==%u\n", samples[x], numargs);
    
    for(int n=0; n<numargs+2; n++) //check a few indices past valid - make sure parser is rejecting out-of-bounds
    {
      char* arg = cmdGetArg((char*)samples[x],n);      
      if( n < numargs ) {
        //these should all be valid args. valid arg ptr, anyway. 0 length is valid!
        if( arg )
          DBG_PRINTF("  arg[%u] len=%u '%s'\n", n, strlen(arg), arg);
        else
          DBG_PRINTF("  arg[%u] FAULT: EXPECTED NON-NULL\n", n);
      } else {
        //parser should be returning NULL for everything here
        if( arg )
          DBG_PRINTF("  arg[%u] len=%u FAULT: EXPECTED NULL: '%s'\n", n, strlen(arg), arg);
      }
    }
  }
  DBG_PRINTF("\n");
#endif
}

//-----------------------------------------------------------------------------
//                  Robot (Charge Contacts)
//-----------------------------------------------------------------------------

#include "emrf.h"

static struct { //result of most recent command
  error_t ccr_err;
  int handler_cnt;
  int sr_cnt;
  union {
    ccr_esn_t esn;
    ccr_bsv_t bsv;
    ccr_sr_t  sr;
  }rsp;
} m_dat;

void esn_handler_(char* s) {
  m_dat.handler_cnt++;
  m_dat.rsp.esn.esn = cmdParseHex32(cmdGetArg(s,0));
  if( errno != 0 )
    m_dat.ccr_err = ERROR_TESTPORT_RSP_BAD_ARG;
}

ccr_esn_t* cmdRobotEsn()
{
  memset( &m_dat, 0, sizeof(m_dat) );
  const char *cmd = "esn 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 150, CMD_OPTS_DEFAULT, esn_handler_);
  
  ConsolePrintf("esn=%08x\n", m_dat.rsp.esn.esn);
  
  if( m_dat.handler_cnt != 1 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("ESN ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    //throw m_dat.ccr_err;
    return 0;
  }
  return &m_dat.rsp.esn;
}

void bsv_handler_(char* s)
{
  uint32_t *dat = (uint32_t*)&m_dat.rsp.bsv;
  if( m_dat.handler_cnt <= 8 )
  {
    dat[m_dat.handler_cnt+0] = cmdParseHex32(cmdGetArg(s,0));
    if( errno != 0 )
      m_dat.ccr_err = ERROR_TESTPORT_RSP_BAD_ARG;
    
    dat[m_dat.handler_cnt+1] = cmdParseHex32(cmdGetArg(s,1));
    if( errno != 0 )
      m_dat.ccr_err = ERROR_TESTPORT_RSP_BAD_ARG;
  }
  m_dat.handler_cnt += 2;
}

ccr_bsv_t* cmdRobotBsv()
{
  memset( &m_dat, 0, sizeof(m_dat) );
  cmdSend(CMD_IO_CONTACTS, "bsv 00 00 00 00 00 00", 200, CMD_OPTS_DEFAULT, bsv_handler_);
  
  ConsolePrintf("bsv hw.rev,model %u %u\n", m_dat.rsp.bsv.hw_rev, m_dat.rsp.bsv.hw_model );
  ConsolePrintf("bsv ein %08x %08x %08x %08x\n", m_dat.rsp.bsv.ein[0], m_dat.rsp.bsv.ein[1], m_dat.rsp.bsv.ein[2], m_dat.rsp.bsv.ein[3] );
  ConsolePrintf("bsv app vers %08x %08x %08x %08x\n", m_dat.rsp.bsv.app_version[0], m_dat.rsp.bsv.app_version[1], m_dat.rsp.bsv.app_version[2], m_dat.rsp.bsv.app_version[3] );
  
  if( m_dat.handler_cnt != 10 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("BSV ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    //throw m_dat.ccr_err;
    return 0;
  }
  return &m_dat.rsp.bsv;
}

void sensor_handler_(char* s)
{
  m_dat.handler_cnt++;
  //m_dat.ccr_err = ERROR_OK; //we only care about the final reading
  
  for(int x=0; x < m_dat.sr_cnt; x++) {
    uint32_t dat = cmdParseHex32(cmdGetArg(s,x));
    m_dat.rsp.sr.val[x] = dat;
    if( dat > 0xffff || errno != 0 )
      m_dat.ccr_err = ERROR_TESTPORT_RSP_BAD_ARG;
  }
}

static ccr_sr_t* cmdRobot_MotGet_(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, const char* cmd)
{
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x %02x", cmd, NN, sensor, (uint8_t)treadL, (uint8_t)treadR, (uint8_t)lift, (uint8_t)head);

  m_dat.sr_cnt = sensor > 8 ? 0 : ccr_sr_cnt[sensor]; //expected number of sensor values per drop/line
  cmdSend(CMD_IO_CONTACTS, b, 150 + (int)NN*6, CMD_OPTS_DEFAULT, sensor_handler_);
  
  ConsolePrintf("sr vals %04x %04x %04x %04x\n", m_dat.rsp.sr.val[0], m_dat.rsp.sr.val[1], m_dat.rsp.sr.val[2], m_dat.rsp.sr.val[3] );
  
  if( m_dat.handler_cnt != NN || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("SR ERROR %i, handler cnt %i/%i\n", m_dat.ccr_err, m_dat.handler_cnt, NN);
    //throw m_dat.ccr_err;
    return 0;
  }
  return &m_dat.rsp.sr;
}
ccr_sr_t* cmdRobotMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head) {
  return cmdRobot_MotGet_(NN, sensor, treadL, treadR, lift, head, "mot");
}
ccr_sr_t* cmdRobotGet(uint8_t NN, uint8_t sensor) {
  return cmdRobot_MotGet_(NN, sensor, 0, 0, 0, 0, "get");
}

void cmdRobotFcc(uint8_t mode, uint8_t cn) {
  char b[22]; const int bz = sizeof(b);
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"fcc %02x %02x 00 00 00 00", mode, cn), 150);
}

void cmdRobotRlg(uint8_t idx) {
  ConsolePrintf("XXX: rlg not implemented\n");
  throw ERROR_EMPTY_COMMAND;
}

void cmdRobot_IdxVal32_(uint8_t idx, uint32_t val, const char* cmd) {
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x 00", cmd, idx, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF );
  cmdSend(CMD_IO_CONTACTS, b, 150);
}
void cmdRobotEng(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "eng"); }
void cmdRobotLfe(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "lfe"); }
void cmdRobotSmr(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "smr"); }

void gmr_handler_(char* s) {
  m_dat.handler_cnt++;
  m_dat.rsp.esn.esn = cmdParseHex32(cmdGetArg(s,0));
  if( errno != 0 )
    m_dat.ccr_err = ERROR_TESTPORT_RSP_BAD_ARG;
}

////#define EMR_FIELD_OFS(fieldname)    offsetof((fieldname),Anki::Cozmo::Factory::EMR)
uint32_t cmdRobotGmr(uint8_t idx)
{
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"gmr %02x 00 00 00 00 00", idx), 150, CMD_OPTS_DEFAULT, gmr_handler_);
  
  ConsolePrintf("gmr=%08x\n", m_dat.rsp.esn.esn);
  
  if( m_dat.handler_cnt != 1 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("GMR ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    //throw m_dat.ccr_err;
    return 0;
  }
  return m_dat.rsp.esn.esn;
}

//-----------------------------------------------------------------------------
//                  Additional Cmd + response parsing
//-----------------------------------------------------------------------------

#define EMMCDL_VER_MAX_LEN  20
static char emmcdl_version[EMMCDL_VER_MAX_LEN+1];
static int emmcdl_read_cnt;

void emmcdl_ver_handler_(char* s) {
  strncpy(emmcdl_version, s, EMMCDL_VER_MAX_LEN);
  emmcdl_version[EMMCDL_VER_MAX_LEN] = '\0';
  emmcdl_read_cnt++;
}

char* cmdGetEmmcdlVersion(int timeout_ms)
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

