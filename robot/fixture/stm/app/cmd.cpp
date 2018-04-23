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
static io_err_t m_ioerr;
static void get_errs_(cmd_io io, io_err_t *ioe) {
  ioe->rxDroppedChars   = io==CMD_IO_DUT_UART ? DUT_UART::getRxDroppedChars()   : (io==CMD_IO_CONTACTS ? Contacts::getRxDroppedChars()   : 0);
  ioe->rxOverflowErrors = io==CMD_IO_DUT_UART ? DUT_UART::getRxOverflowErrors() : (io==CMD_IO_CONTACTS ? Contacts::getRxOverflowErrors() : 0);
  ioe->rxFramingErrors  = io==CMD_IO_DUT_UART ? DUT_UART::getRxFramingErrors()  : (io==CMD_IO_CONTACTS ? Contacts::getRxFramingErrors()  : 0);
}

const int line_maxlen = 126;
static char m_line[line_maxlen+1];
static int m_line_len = 0, linelen_ = 0;
static char* getline_(int c)
{
  if( c == '\r' || c == '\n' ) {
    linelen_ = m_line_len; //save length
    m_line[m_line_len] = '\0';
    m_line_len = 0;
    return m_line;
  } else if( c > 0 && c < 128 && m_line_len < line_maxlen ) //ascii (ignore null)
    m_line[m_line_len++] = c;
  
  return NULL;
}

static int getchar_(cmd_io io) {
  switch(io) {
    case CMD_IO_DUT_UART: return DUT_UART::getchar(); //break;
    case CMD_IO_CONSOLE:  return ConsoleGetCharNoWait(); //break;
    case CMD_IO_CONTACTS: {
      int c = Contacts::getchar();
      if( c == 0x88 ) c = '@'; //syscon debug mode: inserts overflow indicator in the stream
      return c; //break;
    }
    case CMD_IO_SIMULATOR:
    default: return -1; //break;
  }
}

static void flush_(cmd_io io)
{
  m_line[0] = '\0';
  m_line_len = 0;
  linelen_ = 0;
  
  while( getchar_(io) > -1 );
  
  io_err_t ioerrs;
  get_errs_(io, &ioerrs); //read errors to clear
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

char* cmdSend(cmd_io io, const char* scmd, int timeout_ms, int opts, void(*async_handler)(char*) )
{
  char b[80]; const int bz = sizeof(b); //str buffer
  
  //check if we should insert newlines?
  bool newline = scmd[strlen(scmd)-1] != '\n';
  
  if( opts & CMD_OPTS_DBG_PRINT_ENTRY )
    ConsolePrintf("cmdSend(%u, \"%s\"%s, ms=%i)\n", io, scmd, (newline ? "\\n" : ""), timeout_ms);
  
  //contact handoff power -> uart
  if( io == CMD_IO_CONTACTS && Contacts::powerIsOn() ) {
    Contacts::powerOff(); //forces discharge
    Contacts::setModeRx(); //explicit mode change
  }
  
  //send command out the selected io channel (append prefix)
  flush_(io); //flush rx buffers first for correct response detection
  if( io != CMD_IO_CONSOLE && opts & CMD_OPTS_LOG_CMD ) //echo to log BEFORE the DUT write or we'll miss response chars
    write_(CMD_IO_CONSOLE, LOG_CMD_PREFIX, scmd, (newline ? "\n" : NULL) );
  write_(io, CMD_PREFIX, scmd, (newline ? "\n" : NULL) );

  //Get response
  memset( &m_ioerr, 0, sizeof(m_ioerr) ); //clear io errs
  m_status = INT_MIN;
  uint32_t Tstart = Timer::get(), Ttick = Timer::get();
  while( Timer::elapsedUs(Tstart) < timeout_ms*1000 )
  {
    int c = getchar_(io);
    char *rsp = getline_(c);
    
    if( rsp != NULL )
    {
      int rspLen = linelen_; //strlen(rsp);
      
      //find and validate the response
      if( rspLen > sizeof(RSP_PREFIX)-1 && !strncmp(rsp,RSP_PREFIX,sizeof(RSP_PREFIX)-1) ) //response prefix detected
      {
        m_time_ms = Timer::elapsedUs(Tstart)/1000; //time to response rx
        
        rsp += sizeof(RSP_PREFIX)-1; //'strip' prefix
        
        //echo to log (optionally append debug stats)
        if( opts & CMD_OPTS_LOG_RSP ) {
          write_(CMD_IO_CONSOLE, io == CMD_IO_CONSOLE ? RSP_PREFIX : LOG_RSP_PREFIX, rsp); //differentiate prefix depending on the sender
          if( opts & CMD_OPTS_LOG_RSP_TIME )
            write_(CMD_IO_CONSOLE, snformat(b,bz," [%ums]", m_time_ms));
          write_(CMD_IO_CONSOLE, "\n");
        }
        
        //check for rx errors
        get_errs_(io, &m_ioerr);
        if( m_ioerr.rxOverflowErrors > 0 || m_ioerr.rxDroppedChars > 0 ) {
          if( opts & CMD_OPTS_LOG_ERRORS )
            write_(CMD_IO_CONSOLE, snformat(b,bz,"IO ERROR ovfl=%i dropRx=%i frame=%i", m_ioerr.rxOverflowErrors, m_ioerr.rxDroppedChars, m_ioerr.rxFramingErrors));
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
      else if( rspLen > sizeof(ASYNC_PREFIX)-1 && !strncmp(rsp,ASYNC_PREFIX,sizeof(ASYNC_PREFIX)-1) )
      {
        if( io != CMD_IO_CONSOLE && opts & CMD_OPTS_LOG_ASYNC ) //echo to log, unchanged
          write_(CMD_IO_CONSOLE, rsp, "\n");
        
        //async data while waiting for cmd completion
        if( async_handler != NULL )
          async_handler(rsp + sizeof(ASYNC_PREFIX)-1);
      }
      else //uncategorized, informational
      {
        if( io != CMD_IO_CONSOLE && opts & CMD_OPTS_LOG_OTHER ) //echo to log, unchanged
          write_(CMD_IO_CONSOLE, rsp, "\n");
      }
      
    }//rsp != NULL
  }//while( Timer::elapsedUs(Tstart)...
  
  if( opts & CMD_OPTS_DBG_PRINT_RX_PARTIAL ) //see what's leftover in the rx buffer
  {
    if( m_line_len > 0 ) {
      write_(CMD_IO_CONSOLE, ".DBG RX PARTIAL '");
      for(int x=0; x<m_line_len; x++)
        write_(CMD_IO_CONSOLE, snformat(b,bz,"%c", m_line[x] >= ' ' && m_line[x] < '~' ? m_line[x] : '*'));
      write_(CMD_IO_CONSOLE, snformat(b,bz,"' (%i)\n", m_line_len));
    }
  }
  
  //check for rx errors
  get_errs_(io, &m_ioerr);
  if( m_ioerr.rxOverflowErrors > 0 || m_ioerr.rxDroppedChars > 0 ) {
    if( opts & CMD_OPTS_LOG_ERRORS )
      write_(CMD_IO_CONSOLE, snformat(b,bz,"IO ERROR ovfl=%i dropRx=%i frame=%i", m_ioerr.rxOverflowErrors, m_ioerr.rxDroppedChars, m_ioerr.rxFramingErrors));
    ERROR_HANDLE("IO_ERROR\n", ERROR_TESTPORT_RX_ERROR);
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

