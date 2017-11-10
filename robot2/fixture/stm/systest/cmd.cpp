#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "board.h"
#include "cmd.h"
#include "contacts.h"
#include "timer.h"

//-----------------------------------------------------------
//        Line Parsing (hack, src copy from cube console)
//-----------------------------------------------------------
//Note: valid command strings must guarantee no \r \n chars and one valid \0 terminator

static inline bool is_whitespace_(char c) {
  return c <= ' ' || c > '~'; //space or non-printable char
}

//seek to start of next argument.
static char* next_arg_(char* s)
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
  while( !is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  //finally, seek start of next arg
  while( is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  return s; //now pointing to first char of next arg (maybe an opening ")
}

char* console_getarg(char *s, int n, char* buf, int buflen)
{
  if( !s || !buf )
    return NULL;
  
  if( is_whitespace_(*s) ) //need initial seek to start from 0th arg
    n++;
  while(n--) { //seek to nth arg
    if( (s = next_arg_(s)) == NULL )
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
    } while(*++temp != NULL && !is_whitespace_(*temp) );
  }
  
  //copy to output buffer
  len = len > buflen-1 ? buflen-1 : len; //limit to buffer size
  memcpy(buf, s, len );
  buf[len] = '\0';
  return buf;
}

char* console_getargl(char *s, int n)
{
  const int bufsize = 39;
  static char buf[bufsize+1];
  return console_getarg(s, n, buf, sizeof(buf));
}

int console_num_args(char *s)
{
  if(!s)
    return 0;
  
  int n = is_whitespace_(*s) ? 0 : 1;
  while( (s = next_arg_(s)) != NULL )
    n++;
  return n;
}

//-----------------------------------------------------------
//        Output
//-----------------------------------------------------------

static void writes_(const char *s) { 
  if(s) Contacts::write(s);
}

static int respond_(char* cmd, int status, const char* info)
{
  char b[10]; int bz = sizeof(b);
  
  //<<cmd # [info]
  writes_(RSP_PREFIX);
  writes_(cmd);
  writes_( snformat(b,bz," %i ",status) );
  writes_(info);
  writes_("\n");
  
  return status;
}

//-----------------------------------------------------------
//        Commands
//-----------------------------------------------------------

int cmd_process(char* s)
{
  char b[50]; int bz = sizeof(b);
  
  if(!s)
    return STATUS_NULL;
  
  //is this a command?
  int len = strlen(s);
  if( !strncmp(s, CMD_PREFIX, sizeof(CMD_PREFIX)-1) && len > sizeof(CMD_PREFIX)-1 ) //valid cmd formatting
    s += sizeof(CMD_PREFIX)-1; //skip the prefix for parsing
  else
    return STATUS_NOT_A_CMD;
  
  //copy command arg into local buffer
  char cmd_buf[10], *cmd = console_getarg(s, 0, cmd_buf, sizeof(cmd_buf));
  
  //================
  //get version
  //>>getvers
  //<<getvers [status] [0xversion]
  //================
  if( !strcmp(cmd, "getvers") ) {
    writes_("project = systest\n");
    writes_(__DATE__ " " __TIME__ "\n");
    return respond_(cmd, STATUS_OK, snformat(b,bz,"hw=%s",HWVERS) );
  }//-*/
  
  //==========================
  //Set LEDs
  //>>leds [bitfield<n>]
  //<<leds [status]
  //==========================
  if( !strcmp(cmd, "leds") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int ledbf = strtol(console_getargl(s,1), 0,0); //hex ok - must have 0x prefix for cstd parser
    //XXX: hook into backpack driver?
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, "0x%x", ledbf) );
  }//-*/
    
  //==========================
  //Delay (for testing)
  //>>delay [#ms]
  //<<delay 0
  //==========================
  if( !strcmp(cmd, "delay") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int ms = strtol(console_getargl(s,1), 0,0); //hex ok - must have 0x prefix for cstd parser
    writes_( snformat(b,bz,"spin for %ims\n", ms) );
    
    while(ms--) {
      Timer::wait(1000);
      if( ms > 0 && ms%1000 == 0)
        writes_( snformat(b,bz,"%us\n",ms/1000) );
    }
    
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  //==========================
  //Console Echo
  //>>echo {off/on}
  //==========================
  if( !strcmp(cmd, "echo") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int echo = -1;
    char* arg1 = console_getargl(s,1);
    if( !strcmp(arg1, "off") )
      Contacts::echo( echo=0 );
    else if( !strcmp(arg1, "on") )
      Contacts::echo( echo=1 );
    
    if( echo > -1 )
      return respond_(cmd, STATUS_OK, arg1);
    else
      return respond_(cmd, STATUS_ARG_PARSE, "bad arg");
  }//-*/
  
  //==========================
  //VDD
  //>>vdd {0/1}
  //==========================
  if( !strcmp(cmd, "vdd") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    //since this will probably shut off the mcu, respond before enacting
    int on = strtol(console_getargl(s,1), 0,0);
    int rsp = respond_(cmd, STATUS_OK, snformat(b,bz, on ? "on" : "off") );
    
    Board::vdd( on > 0 );
    return rsp;
  }//-*/
  
  //==========================
  //VBATs
  //>>vbats {on,vbat,vext,off}
  //==========================
  if( !strcmp(cmd, "vbats") )
  {
    int nargs = console_num_args(s);
    if( nargs < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    if( !strcmp(console_getargl(s,1), "vbat") || !strcmp(console_getargl(s,1), "on")) {
      Board::vbats(VBATS_SRC_VBAT);
      snformat(b,bz,"on,vbat");
    } else if( !strcmp(console_getargl(s,1), "vext") ) {
      Board::vbats(VBATS_SRC_VEXT);
      snformat(b,bz,"on,vext");
    } else { //"off", or any unrecognized param
      Board::vbats(VBATS_SRC_OFF);
      snformat(b,bz,"off");
    }
    
    return respond_(cmd, STATUS_OK, b );
  }//-*/
  
  //==========================
  //battery charger ctrl
  //>>charger {off,on,low,high}
  //==========================
  if( !strcmp(cmd, "charger") )
  {
    int nargs = console_num_args(s);
    if( nargs < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    if( !strcmp(console_getargl(s,1), "on") || !strcmp(console_getargl(s,1), "low")) {
      Board::charger(CHRG_LOW);
      snformat(b,bz,"on,low");
    } else if( !strcmp(console_getargl(s,1), "high") ) {
      Board::charger(CHRG_HIGH);
      snformat(b,bz,"on,high");
    } else { //"off", or any unrecognized param
      Board::charger(CHRG_HIGH);
      snformat(b,bz,"off");
    }
    
    return respond_(cmd, STATUS_OK, b );
  }//-*/
  
  //==========================
  //Unknown (catch-all)
  //==========================
  return respond_(cmd, STATUS_UNKNOWN_CMD, "unknown");
}

