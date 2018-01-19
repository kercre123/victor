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

static void flush_(cmd_io io)
{
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
  
  switch(io)
  {
    case CMD_IO_DUT_UART:
      rsp = DUT_UART::getline(rsp_buf, sizeof(rsp_buf), timeout_us);
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

char* cmdSend(cmd_io io, const char* scmd, int timeout_ms, int opts, void(*async_handler)(char*) )
{
  char b[40]; int bz = sizeof(b); //str buffer
  
  //check if we should insert newlines?
  bool newline = scmd[strlen(scmd)-1] != '\n';
  
  if( opts & CMD_OPTS_DBG_PRINT_ENTRY )
    ConsolePrintf("cmdSend(%u, \"%s\"%s, ms=%i)\n", io, scmd, (newline ? "\\n" : ""), timeout_ms);
  
  //send command out the selected io channel (append prefix)
  flush_(io); //flush rx buffers first for correct response detection
  write_(io, CMD_PREFIX, scmd, (newline ? "\n" : NULL) );
  if( io != CMD_IO_CONSOLE ) //echo to log
    write_(CMD_IO_CONSOLE, LOG_CMD_PREFIX, scmd, (newline ? "\n" : NULL) );

  //Get response
  m_status = INT_MIN;
  uint32_t Tstart = Timer::get();
  while( Timer::elapsedUs(Tstart) < timeout_ms*1000 )
  {
    char *rsp;
    if( (rsp = getline_(io, timeout_ms*1000 - Timer::elapsedUs(Tstart) )) != NULL )
    {
      int rspLen = strlen(rsp);
      
      //find and validate the response
      if( rspLen > sizeof(RSP_PREFIX)-1 && !strncmp(rsp,RSP_PREFIX,sizeof(RSP_PREFIX)-1) ) //response prefix detected
      {
        m_time_ms = Timer::elapsedUs(Tstart)/1000; //time to response rx
        
        rsp += sizeof(RSP_PREFIX)-1; //'strip' prefix
        if( io != CMD_IO_CONSOLE ) //echo to log with modified prefix
          write_(CMD_IO_CONSOLE, LOG_RSP_PREFIX, rsp, "\n");
        if( opts & CMD_OPTS_DBG_PRINT_RSP_TIME )
          write_(CMD_IO_CONSOLE, snformat(b,bz,"cmd time %ums\n", m_time_ms));
        
        //make sure response matches our command word
        int nargs = cmdNumArgs(rsp);
        if( nargs < 1 || strcmp( cmdGetArg((char*)scmd,0,b,bz), cmdGetArg(rsp,0)) > 0 ) {
          ERROR_HANDLE("MISMATCH\n", ERROR_TESTPORT_RSP_MISMATCH);
          return NULL;
        }
        
        //enforce valid status code as first rsp arg
        if( nargs < 2 && opts & CMD_OPTS_REQUIRE_STATUS_CODE ) {
          //validate numeric?
          ERROR_HANDLE("MISSING ARGS\n", ERROR_TESTPORT_RSP_MISSING_ARGS);
          return NULL;
        }

        //try parse the 2nd arg as status code (with error checking)
        if( nargs >= 2 )
        {
          errno = 0;
          char *endptr, *argstatus = cmdGetArg(rsp,1);
          long val = strtol(argstatus, &endptr, 10); //enforce base10
          
          //check conversion errs, limit numerical range, and verify entire arg was parsed (stops at whitespace, invalid chars...)
          if( errno == 0 && val <= INT_MAX && val >= INT_MIN+1 && endptr > argstatus && *endptr == '\0' )
            m_status = val; //record parsed val
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

