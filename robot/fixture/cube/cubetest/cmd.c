#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "app.h"
#include "accel.h"
#include "board.h"
#include "cmd.h"
#include "console.h"
#include "hal_led.h"
#include "hal_timer.h"
#include "leds.h"
#include "vbat.h"

//-----------------------------------------------------------
//        Output
//-----------------------------------------------------------

static void writes_(const char *s) { 
  if(s) console_write((char*)s); 
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
    writes_("cubetest build " __DATE__ " " __TIME__ "\n");
    return respond_(cmd, STATUS_OK, snformat(b,bz,"hw=%s",HWVERS) );
  }//-*/
  
  //==========================
  //read battery voltage(s)
  //>>vbat
  //<<vbat [status] VBAT1V-mV [voltage-mv] VBAT3V-mV [voltage-mv]
  //==========================
  if( !strcmp(cmd, "vbat") ) {
    return respond_(cmd, STATUS_OK, snformat(b,bz,"VBAT1V-mV %u VBAT3V-mV %u", get_vbat1v_mv(), get_vbat3v_mv()) );
  }//-*/
  
  //==========================
  //Set LEDs
  //>>leds [bitfield<n>] [static]
  //<<leds [status]
  //==========================
  if( !strcmp(cmd, "leds") )
  {
    int nargs = console_num_args(s);
    if( nargs < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    //command bitfield separates RGB leds by nibble
    //bitfield<3:0>   D1.{xBGR}
    //bitfield<7:4>   D2.{xBGR}
    //bitfield<11:8>  D3.{xBGR}
    //bitfield<15:12> D4.{xBGR}
    
    int bf = strtol(console_getargl(s,1), 0,0); //hex ok - must have 0x prefix for cstd parser
    
    //'leds' API uses packed bitfield
    int ledbf = ((bf&0x7000)>>3) | ((bf&0x0700)>>2) | ((bf&0x0070)>>1) | ((bf&0x0007)>>0);
    
    //'static' specifier uses hal layer, no duty cycling, only 1 led on at a time
    if( ledbf > 0 && nargs >= 3 && !strcmp(console_getargl(s,2), "static") ) {
      leds_set(0); //disable duty-cycled timer
      uint16_t n = 0;
      while( (ledbf & (1<<n)) == 0 ) { n++; }
      //writes_( snformat(b,bz,"bf %04x -> %04x -> hal_led_on(%i)\n", bf, ledbf, n) );
      hal_led_on( n );
    } else {
      hal_led_off();
      leds_set( ledbf & LED_BF_ALL );
    }
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, "0x%x", ledbf & LED_BF_ALL) );
  }//-*/
  
  //==========================
  //VLED boost reg en
  //>>vled {0/1}
  //<<vled [status]
  //==========================
  if( !strcmp(cmd, "vled") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    bool on = strtol(console_getargl(s,1), 0,0) > 0;
    hal_led_power(on);
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, "%s", on ? "on" : "off") );
  }//-*/
  
  //==========================
  //Sleep. Put MCU into low-power mode
  //>>sleep [mode]
  //<<sleep [status]
  //==========================
  if( !strcmp(cmd, "sleep") )
  {
    int mode = 0;
    if( console_num_args(s) >= 2 )
      mode = strtol(console_getargl(s,1), 0,0);
    
    //respond before going to sleep
    int retval = respond_(cmd, STATUS_OK, snformat(b,bz,"mode=%u", mode) );
    hal_uart_tx_flush(); //wait for all dat to be sent out TX port
    
    // In extended or deep sleep mode the watchdog timer is disabled
    // (power domain PD_SYS is automatically OFF). Although, if the debugger
    // is attached the watchdog timer remains enabled and must be explicitly
    // disabled.
    if ((GetWord16(SYS_STAT_REG) & DBG_IS_UP) == DBG_IS_UP) {
      SetWord16(SET_FREEZE_REG, FRZ_WDOG);  //wdg_freeze();
    }
    
    WFI(); __nop(); __nop();
    //--------- wakeup -----------
    return retval;
    
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
      hal_timer_wait(1000);
      if( ms > 0 && ms%1000 == 0)
        writes_( snformat(b,bz,"%us\n",ms/1000) );
    }
    
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  //==========================
  //Test Accelerometer
  //>>accel
  //==========================
  if( !strcmp(cmd, "accel") )
  {
    accel_power_off();
    if( accel_chipinit() ) { //this should fail!
      accel_power_off();
      return respond_(cmd, STATUS_ACCEL_PWR_CTRL, "pwr_ctrl");
    }
    
    accel_power_on();
    if( !accel_chipinit() ) { //this should pass
      return respond_(cmd, STATUS_ACCEL_CHIPINIT, "chipinit");
    }
    
    //XXX: read some data & ?sanity check
    
    accel_power_off();
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
    
    char* arg1 = console_getargl(s,1);
    if( !strcmp(arg1, "off") )
      console_echo( g_echo_on=0 );
    else if( !strcmp(arg1, "on") )
      console_echo( g_echo_on=1 );
    else
      return respond_(cmd, STATUS_ARG_PARSE, "bad arg");
    
    return respond_(cmd, STATUS_OK, arg1);
  }//-*/
  
  //==========================
  //Unknown (catch-all)
  //==========================
  return respond_(cmd, STATUS_UNKNOWN_CMD, "unknown");
}

