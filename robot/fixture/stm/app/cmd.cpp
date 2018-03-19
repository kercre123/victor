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
#include "timer.h"

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

typedef struct { int rxDroppedChars; int rxOverflowErrors; int rxFramingErrors; } io_err_t;
static void get_errs_(cmd_io io, io_err_t *ioe) {
  ioe->rxDroppedChars   = io==CMD_IO_DUT_UART ? DUT_UART::getRxDroppedChars()   : (io==CMD_IO_CONTACTS ? Contacts::getRxDroppedChars()   : 0);
  ioe->rxOverflowErrors = io==CMD_IO_DUT_UART ? DUT_UART::getRxOverflowErrors() : (io==CMD_IO_CONTACTS ? Contacts::getRxOverflowErrors() : 0);
  ioe->rxFramingErrors  = io==CMD_IO_DUT_UART ? DUT_UART::getRxFramingErrors()  : (io==CMD_IO_CONTACTS ? Contacts::getRxFramingErrors()  : 0);
}

static char rsp_buf[100];
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
  
  //read errors to clear
  io_err_t ioerrs;
  get_errs_(io, &ioerrs);
}

static char* getline_(cmd_io io, int timeout_us)
{
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

static char* getline_raw_(cmd_io io, int *out_len) //buffer scraps for debug when cmd times out
{
  switch(io)
  {
    case CMD_IO_DUT_UART:
      if( out_len )
        *out_len = rsp_len;
      return rsp_buf;
    case CMD_IO_CONTACTS:
      return Contacts::getlinebuffer(out_len);
    default:
    case CMD_IO_SIMULATOR:
    case CMD_IO_CONSOLE: //XXX: dummy buffer for console (too lazy to update API)
      if( out_len )
        *out_len = 0;
      rsp_buf[0] = '\0';
      return rsp_buf;
  }
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
  char b[80]; const int bz = sizeof(b); //str buffer
  
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
        
        //check for rx errors
        io_err_t ioerr;
        get_errs_(io, &ioerr);
        if( ioerr.rxOverflowErrors > 0 || ioerr.rxDroppedChars > 0 ) {
          write_(CMD_IO_CONSOLE, snformat(b,bz,"--> IO ERROR ovfl=%i dropRx=%i frame=%i", ioerr.rxOverflowErrors, ioerr.rxDroppedChars, ioerr.rxFramingErrors));
          ERROR_HANDLE("IO_ERROR\n", ERROR_TESTPORT_RX_ERROR);
          //return NULL; //if no exception, allow cmd validation to continue
        }
        
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
  
  //DEBUG: see what's leftover in the rx buffer
  int rlen;
  char* rawline = getline_raw_(io, &rlen);
  if( rlen > 0 ) {
    write_(CMD_IO_CONSOLE, ".DBG RX PARTIAL '");
    for(int x=0; x<rlen; x++)
      write_(CMD_IO_CONSOLE, snformat(b,bz,"%c", rawline[x] >= ' ' && rawline[x] < '~' ? rawline[x] : '*'));
    write_(CMD_IO_CONSOLE, snformat(b,bz,"' (%i)\n", rlen));
  }
  
  //check for rx errors
  io_err_t ioerr;
  get_errs_(io, &ioerr);
  if( ioerr.rxOverflowErrors > 0 || ioerr.rxDroppedChars > 0 ) {
    write_(CMD_IO_CONSOLE, snformat(b,bz,"--> IO ERROR ovfl=%i dropRx=%i frame=%i", ioerr.rxOverflowErrors, ioerr.rxDroppedChars, ioerr.rxFramingErrors));
    ERROR_HANDLE("IO_ERROR\n", ERROR_TESTPORT_RX_ERROR);
    //return NULL; //if no exception, allow cmd validation to continue
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

int32_t cmdParseInt32(char *s)
{
  errno = -1;
  if(s)
  {
    errno = 0;
    char *endptr;
    long val = strtol(s, &endptr, 10); //enforce base10

    //check conversion errs, limit numerical range, and verify entire arg was parsed (stops at whitespace, invalid chars...)
    if( errno == 0 && val <= INT_MAX && val >= INT_MIN/*+1*/ && endptr > s && *endptr == '\0' )
      return val;
  }
  return INT_MIN; //parse error
}

uint32_t cmdParseHex32(char* s)
{
  errno = -1;
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
      if( errno == 0 )
      {
        buf[len-4] = '\0'; //remove low
        uint32_t high = strtol(buf,0,16);
        if( errno == 0 )
          return (high << 16) | low;
      }
    }
  }
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

#define CCC_DEBUG 1
#define CCC_ERROR_HANDLE(e) if( CCC_DEBUG > 0 ) { return 0; } else { throw (e); }
#define CCC_CMD_DELAY()     if( CCC_DEBUG > 0 ) { Timer::delayMs(150); }

//buffer for sensor data
static uint8_t *srbuf = app_global_buffer;
static const int srdatMaxSize = sizeof(ccr_sr_t) * 256;
STATIC_ASSERT( APP_GLOBAL_BUF_SIZE >= srdatMaxSize , cmd_get_sensor_buffer_size_check );

#include "emrf.h"

static struct { //result of most recent command
  error_t ccr_err;
  int handler_cnt;
  int sr_cnt;
  union { 
    ccr_esn_t esn; 
    ccr_bsv_t bsv;
  } rsp;
} m_dat;

void esn_handler_(char* s) {
  m_dat.rsp.esn.esn = cmdParseHex32( cmdGetArg(s,0) );
  m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
  m_dat.handler_cnt++;
}

ccr_esn_t* cmdRobotEsn()
{
  CCC_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  const char *cmd = "esn 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CMD_OPTS_DEFAULT, esn_handler_);
  
  #if CCC_DEBUG > 0
  ConsolePrintf("esn=%08x\n", m_dat.rsp.esn.esn );
  #endif
  
  if( m_dat.handler_cnt != 1 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("ESN ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    CCC_ERROR_HANDLE(m_dat.ccr_err);
  }
  return &m_dat.rsp.esn;
}

void bsv_handler_(char* s)
{
  uint32_t *dat = (uint32_t*)&m_dat.rsp; //esn,bsv are both uint32_t arrays
  const int nwords_max = sizeof(m_dat.rsp)/sizeof(uint32_t);
  
  if( m_dat.handler_cnt <= nwords_max-2 ) {
    dat[m_dat.handler_cnt+0] = cmdParseHex32( cmdGetArg(s,0) );
    m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
    dat[m_dat.handler_cnt+1] = cmdParseHex32(cmdGetArg(s,1));
    m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG: m_dat.ccr_err;
  }
  m_dat.handler_cnt += 2;
}

ccr_bsv_t* cmdRobotBsv()
{
  CCC_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  const char *cmd = "bsv 00 00 00 00 00 00";
  cmdSend(CMD_IO_CONTACTS, cmd, 500, CMD_OPTS_DEFAULT, bsv_handler_);
  
  #if CCC_DEBUG > 0
  ConsolePrintf("bsv hw.rev,model %u %u\n", m_dat.rsp.bsv.hw_rev, m_dat.rsp.bsv.hw_model );
  ConsolePrintf("bsv ein %08x %08x %08x %08x\n", m_dat.rsp.bsv.ein[0], m_dat.rsp.bsv.ein[1], m_dat.rsp.bsv.ein[2], m_dat.rsp.bsv.ein[3] );
  ConsolePrintf("bsv app vers %08x %08x %08x %08x\n", m_dat.rsp.bsv.app_version[0], m_dat.rsp.bsv.app_version[1], m_dat.rsp.bsv.app_version[2], m_dat.rsp.bsv.app_version[3] );
  #endif
  
  if( m_dat.handler_cnt != 10 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("BSV ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    CCC_ERROR_HANDLE(m_dat.ccr_err);
  }
  return &m_dat.rsp.bsv;
}

void sensor_handler_(char* s) {
  ccr_sr_t *psr = &((ccr_sr_t*)srbuf)[m_dat.handler_cnt++];
  if( m_dat.handler_cnt <= 256 ) { //don't overflow buffer
    for(int x=0; x < m_dat.sr_cnt; x++) {
      psr->val[x] = cmdParseInt32( cmdGetArg(s,x) );
      m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_dat.ccr_err;
    }
  }
}

static ccr_sr_t* cmdRobot_MotGet_(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head, const char* cmd)
{
  CCC_CMD_DELAY();
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x %02x", cmd, NN, sensor, (uint8_t)treadL, (uint8_t)treadR, (uint8_t)lift, (uint8_t)head);
  
  memset( &m_dat, 0, sizeof(m_dat) );
  memset( srbuf, 0, (1+NN)*sizeof(ccr_sr_t) );
  m_dat.sr_cnt = sensor >= sizeof(ccr_sr_cnt) ? 0 : ccr_sr_cnt[sensor]; //expected number of sensor values per drop/line
  if( !NN )
    throw ERROR_BAD_ARG;
  
  cmdSend(CMD_IO_CONTACTS, b, 500 + (int)NN*6, CMD_OPTS_DEFAULT, sensor_handler_);
  
  #if CCC_DEBUG > 0
  ccr_sr_t* psr = (ccr_sr_t*)srbuf;
  ConsolePrintf("NN=%u sr vals %i %i %i %i\n", NN, psr[NN-1].val[0], psr[NN-1].val[1], psr[NN-1].val[2], psr[NN-1].val[3] );
  #endif
  
  if( m_dat.handler_cnt != NN || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("SR ERROR %i, handler cnt %i/%i\n", m_dat.ccr_err, m_dat.handler_cnt, NN);
    CCC_ERROR_HANDLE(m_dat.ccr_err);
  }
  
  return (ccr_sr_t*)srbuf;
}
ccr_sr_t* cmdRobotMot(uint8_t NN, uint8_t sensor, int8_t treadL, int8_t treadR, int8_t lift, int8_t head) {
  return cmdRobot_MotGet_(NN, sensor, treadL, treadR, lift, head, "mot");
}
ccr_sr_t* cmdRobotGet(uint8_t NN, uint8_t sensor) {
  return cmdRobot_MotGet_(NN, sensor, 0, 0, 0, 0, "get");
}

void cmdRobotFcc(uint8_t mode, uint8_t cn) {
  CCC_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  cmdSend(CMD_IO_CONTACTS, snformat(b,bz,"fcc %02x %02x 00 00 00 00", mode, cn), 500);
}

void cmdRobotRlg(uint8_t idx) {
  CCC_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  ConsolePrintf("XXX: rlg not implemented\n");
  throw ERROR_EMPTY_COMMAND;
}

void cmdRobot_IdxVal32_(uint8_t idx, uint32_t val, const char* cmd, void(*handler)(char*) ) {
  CCC_CMD_DELAY();
  memset( &m_dat, 0, sizeof(m_dat) );
  char b[22]; const int bz = sizeof(b);
  snformat(b,bz,"%s %02x %02x %02x %02x %02x 00", cmd, idx, (val>>0)&0xFF, (val>>8)&0xFF, (val>>16)&0xFF, (val>>24)&0xFF );
  cmdSend(CMD_IO_CONTACTS, b, 500, CMD_OPTS_DEFAULT, handler );
}
void cmdRobotEng(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "eng", 0); }
void cmdRobotLfe(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "lfe", 0); }
void cmdRobotSmr(uint8_t idx, uint32_t val) { cmdRobot_IdxVal32_(idx, val, "smr", 0); }

void gmr_handler_(char* s) {
  m_dat.handler_cnt++;
  m_dat.rsp.esn.esn = cmdParseHex32( cmdGetArg(s,0) );
  m_dat.ccr_err = errno != 0 ? ERROR_TESTPORT_RSP_BAD_ARG : m_dat.ccr_err;
}

////#define EMR_FIELD_OFS(fieldname)    offsetof((fieldname),Anki::Cozmo::Factory::EMR)
uint32_t cmdRobotGmr(uint8_t idx)
{
  cmdRobot_IdxVal32_(idx, 0, "gmr", gmr_handler_);
  ConsolePrintf("gmr=%08x\n", m_dat.rsp.esn.esn);
  
  if( m_dat.handler_cnt != 1 || m_dat.ccr_err != ERROR_OK ) {
    ConsolePrintf("GMR ERROR %i, handler cnt %i\n", m_dat.ccr_err, m_dat.handler_cnt);
    CCC_ERROR_HANDLE(m_dat.ccr_err);
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

int cmdGetHelperTempC(int zone)
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

