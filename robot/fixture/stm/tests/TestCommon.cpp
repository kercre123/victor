#include <string.h>
#include "app.h"
#include "board.h"
#include "console.h"
#include "contacts.h"
#include "dut_uart.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "systest.h"
#include "testcommon.h"
#include "tests.h"
#include "timer.h"

//-----------------------------------------------------------------------------
//                  Power Short-Circuit Tests
//-----------------------------------------------------------------------------

namespace TestCommon
{
  //#define DBG_VERBOSE(x)  x
  #define DBG_VERBOSE(x)  {}
  
  void powerOnProtected(pwr_e net, uint16_t time_ms, int ilimit_ma, int oversample)
  {
    int *ilog = (int*)app_global_buffer; //buffer to store sample data
    int ilogSize = APP_GLOBAL_BUF_SIZE / sizeof(int);
    int ima=0, icnt=0, peak=0, sum=0;
    
    uint32_t start = Timer::get();
    Board::powerOn(net,0); //turn power on (no delays)
    while( Timer::get()-start < time_ms*1000 )
    {
      ima = Meter::getCurrentMa(net, oversample);
      
      //log sample & update metrics
      peak = ima > peak ? ima : peak;
      if( icnt < ilogSize ) {
        ilog[icnt++] = ima;
        sum += ima;
      }
      
      //overcurrent
      if( ima > ilimit_ma ) {
        Board::powerOff(net);
        break;
      }
    }
    ConsolePrintf("%s startup current: peak %imA, avg %imA (%u samples)\n", Board::power2str(net), peak, sum/icnt, icnt);
    
    //DEBUG: print current samples
    DBG_VERBOSE(
      for(int i=0; i < icnt; i++) {
        if( i && !(i%16) )
          ConsolePrintf("\n");
        ConsolePrintf("%03i,", ilog[i]);
      }
      ConsolePrintf("\n");
    );
    
    if( ima > ilimit_ma )
      throw ERROR_POWER_SHORT;
  }
}

//-----------------------------------------------------------------------------
//                  Console Bridge
//-----------------------------------------------------------------------------

namespace TestCommon
{
  static inline const char* which_s_(bridge_target_e which) {
    switch(which) {
      case TO_DUT_UART: return "DUT UART"; //break;
      case TO_CONTACTS: return "Charge Contacts"; //break;
      default: return "???"; //break;
    }
  }
  
  static inline int bridge_getchar_(bridge_target_e which) {
    switch(which) {
      case TO_DUT_UART: return DUT_UART::getchar(); //break;
      case TO_CONTACTS: return !Contacts::powerIsOn() ? Contacts::getchar() : -1; //break;
      default: return -1; //break;
    }
  }
  
  static inline void bridge_putchar_(bridge_target_e which, char c, bool half_duplex_stay_tx_mode) {
    switch(which) {
      case TO_DUT_UART: DUT_UART::putchar(c); break;
      case TO_CONTACTS: 
        if( !Contacts::powerIsOn() ) { 
          Contacts::putchar(c);
          if( !half_duplex_stay_tx_mode )
            Contacts::setModeRx(); /*back to rx immediately*/ 
        }
        break;
    }
  }
  
  static inline void bridge_get_errs_(bridge_target_e which, int e[]) {
    e[0] = which == TO_DUT_UART ? DUT_UART::getRxDroppedChars() : Contacts::getRxDroppedChars();
    e[1] = which == TO_DUT_UART ? DUT_UART::getRxOverflowErrors() : Contacts::getRxOverflowErrors();
    e[2] = which == TO_DUT_UART ? DUT_UART::getRxFramingErrors() : Contacts::getRxFramingErrors();
  }
  
  void consoleBridge(bridge_target_e which, int inactivity_delay_ms, int timeout_ms, int opts, bridge_hook_sendline_t hook_sendline )
  {
    const char cmd_exit[5] = "exit";    int exit_idx = 0;
    const char cmd_echo[5] = "echo";    int echo_idx = 0;
    const char cmd_buff[5] = "buff";    int buff_idx = 0;
    const char cmd_stats[6] = "stats";  int stats_idx = 0;
    const char cmd_charge[7] = "charge"; int charge_idx = 0;
    
    bool echo = opts & BRIDGE_OPT_LOCAL_ECHO;
    bool bufmode = opts & BRIDGE_OPT_LINEBUFFER;
    const  int  line_maxlen = 40;
    int  line_len = 0, recall_len = 0;
    char line[line_maxlen+1];
    const int charge_idle_ms = 7*1000; //delay before enabling robot charging
    int idle_ms = 0;
    bool charge_en = (opts & BRIDGE_OPT_CHG_DISABLE) == 0;
    
    ConsolePrintf("Console Bridge <-> %s for %ims,inactive:%ims [or type 'exit']\n", which_s_(which), timeout_ms, inactivity_delay_ms);
    
    if( which == TO_CONTACTS )
      Contacts::setModeRx(); //explicit mode change required?
    
    int c, esc_cnt = 0;
    uint32_t Tstart = Timer::get();
    while(1)
    {
      //timeout/inactivity checks
      if( Timer::elapsedUs(Tstart) >= 1000 )
      {
        Tstart = Timer::get();
        if( timeout_ms > 0 && --timeout_ms == 0 )
          break;
        if( inactivity_delay_ms > 0 && --inactivity_delay_ms == 0 )
          break;        
        if( which == TO_CONTACTS && charge_en ) {
          if( ++idle_ms >= charge_idle_ms && !Contacts::powerIsOn() ) {
            //ConsolePrintf("idle timeout. charging enabled\n");
            Contacts::powerOn(); //Note: prevents rx from contacts until console is re-actviated
            Board::ledOn(Board::LED_GREEN);
          }
        }
      }
      
      //be the bridge you were born to be
      do {        
        if( (c = bridge_getchar_(which)) > -1 ) {
          if( c > 0 && c <= 0x7f ) { //limit to ascii, ignore null. getline() methods usually do this.
            idle_ms = 0;
            ConsolePutChar(c);
          }
        }
      } while( c > -1 );
      
      //process console input
      if( (c = ConsoleReadChar()) > -1 )
      {
        inactivity_delay_ms = 0; //disabled on activity
        if( which == TO_CONTACTS && charge_en ) {
          idle_ms = 0; //reset idle counter
          if( Contacts::powerIsOn() ) {
            //ConsolePrintf("charging disabled\n");
            Contacts::setModeRx(); //re-enable contact uart
            Board::ledOff(Board::LED_GREEN);
          }
        }
        
        if( !bufmode ) { //normal mode
          if( echo )
            ConsolePutChar(c);
          bridge_putchar_(which, c, 0); //immediate write
        }
        else //use line buffering
        {
          if( c > 0 && c < 0x80 ) //ignore null and non-ascii
          {
            if( c == '\r' || c == '\n' ) { //enter or return
              ConsolePutChar(c);
              if( line_len > 0 )
              {
                line[line_len] = '\0';
                bool allow_send = 1;
                
                if( hook_sendline != NULL ) //hook for external alias/substitutions
                {
                  const char* sub_line = hook_sendline(line, line_len);
                  int sub_len = sub_line != NULL ? strlen(sub_line) : 0;
                  
                  //allow hook to finish with newline to flush target rx
                  if( sub_len == 1 && sub_line[0] == '\n' ) {
                    ConsolePutChar('\n');
                    sub_len = 0;
                    sub_line = NULL;
                  }
                  
                  if( sub_line /*&& sub_len > 0*/ )
                  {
                    sub_len = sub_len > line_maxlen ? line_maxlen : sub_len; //limit to our buffer size
                    memcpy(line, sub_line, sub_len);
                    line_len = sub_len;
                    line[line_len] = '\0';
                    
                    /*/DEBUG
                    ConsolePrintf(".sub line '");
                    for(int x=0; x<line_len; x++) {
                      if( line[x] == '\n' ) ConsolePrintf("/n");
                      else ConsolePutChar( line[x] >= ' ' && line[x] < '~' ? line[x] : '*');
                    } ConsolePrintf("' (%i)\n", line_len);
                    //-*/
                    
                    allow_send = 0; //don't send this now; leave in console for user to modify
                    for(int x=0; x<line_len; x++) { //be helpful and type the substitute line into the console
                      ConsolePutChar(line[x]);
                      if( line[x] == '\n' )
                        allow_send = 1; //send if requested!
                    }
                  }
                }
                
                if( allow_send ) {
                  for(int x=0; x<line_len; x++)
                    bridge_putchar_(which, line[x], 1); //dump the entire line (hold TX mode in half-duplex)
                  bridge_putchar_(which, '\n', 0); //print final char and return to RX mode
                  recall_len = line_len; //save length for recall
                  line_len = 0;
                }
              }
            } else if( c == 0x1B ) { //ESC
              if( recall_len > 0 ) {
                for(int x=0; x<recall_len; x++)
                  ConsolePutChar(line[x]); //re-print last command on the console
                line_len = recall_len; //restore buffer point
                recall_len = 0;
              }
            } else if (c == 0x7F || c == 8) {  //delete or backspace
              if( line_len > 0 ) {
                ConsolePutChar(c);      //overwrite the onscreen char with a space
                ConsolePutChar(' ');    //"
                ConsolePutChar(c);      //"
                line[--line_len] = 0;
              }
            } else if( line_len < line_maxlen ) {
              ConsolePutChar(c);
              line[line_len++] = c;
              recall_len = 0;
            }
            //else, whoops! drop data we don't have room for
          }
        }
        
        //AnkiLog firmware update from inside the bridge
        esc_cnt = c == 0x1B ? esc_cnt + 1 : 0; //count consecutive ESC chars
        if( esc_cnt >= 5 ) {
          esc_cnt = 0;
          uint32_t Tanki = Timer::get();
          while( Timer::elapsedUs(Tanki) < 15*1000*1000 )
            ConsoleUpdate();
          ConsolePrintf("\nAborting Firmware Update\n");
        }
        
        //track 'exit' cmd
        if( (c == '\r' || c == '\n') && exit_idx == sizeof(cmd_exit)-1 ) //cmd match w/ EOL
          timeout_ms = 50; //wait around to hear response/echo
        else
          exit_idx = (c == cmd_exit[exit_idx]) ? exit_idx+1 : 0; //index track
        
        //track 'echo' cmd
        if( (c == '\r' || c == '\n') && echo_idx == sizeof(cmd_echo)-1 ) { //cmd match w/ EOL
          echo = !echo;
          ConsolePrintf("echo %s\n", echo ? "on" : "off");
          echo_idx = 0;
        } else
          echo_idx = (c == cmd_echo[echo_idx]) ? echo_idx+1 : 0; //index track
        
        //track 'buff' cmd
        if( (c == '\r' || c == '\n') && buff_idx == sizeof(cmd_buff)-1 ) { //cmd match w/ EOL
          bufmode = !bufmode;
          ConsolePrintf("linebuffer %s\n", bufmode ? "enabled" : "disabled");
          buff_idx = 0;
        } else
          buff_idx = (c == cmd_buff[buff_idx]) ? buff_idx+1 : 0; //index track
        
        //track 'stats' cmd
        if( (c == '\r' || c == '\n') && stats_idx == sizeof(cmd_stats)-1 ) { //cmd match w/ EOL
          struct { int dropped_chars; int overflow; int framing; } e;
          bridge_get_errs_(which, (int*)&e );
          ConsolePrintf("drop:%i ovf:%i frame:%i\n", e.dropped_chars, e.overflow, e.framing );
          stats_idx = 0;
        } else
          stats_idx = (c == cmd_stats[stats_idx]) ? stats_idx+1 : 0; //index track
        
        //track 'charge' cmd
        if( (c == '\r' || c == '\n') && charge_idx == sizeof(cmd_charge)-1 ) { //cmd match w/ EOL
          charge_en = !charge_en;
          ConsolePrintf("charge_en=%u\n", charge_en);
          if( which == TO_CONTACTS && !charge_en && Contacts::powerIsOn() ) {
            Contacts::setModeRx(); //re-enable contact uart
            Board::ledOff(Board::LED_GREEN);
          }
          charge_idx = 0;
        } else
          charge_idx = (c == cmd_charge[charge_idx]) ? charge_idx+1 : 0; //index track
        
      }//c > -1
      
    }
    
    if( which == TO_CONTACTS ) {
      Contacts::setModeRx(); //return to idle state
      Board::ledOff(Board::LED_GREEN);
    }
    ConsolePrintf("Console restored\n");
  }
  
  int waitForKeypress(int timeout, bool flush_rx, const char *printstr)
  {
    if( printstr )
      ConsoleWrite((char*)printstr);
    if( flush_rx )
      while( ConsoleReadChar() > -1 );
    
    uint32_t Tstart = Timer::get();
    do {
      if( ConsoleReadChar() > -1 )
        return 0;
    } while( timeout==0 || (timeout>0 && Timer::elapsedUs(Tstart) < timeout) );
    
    return 1;
  }
  
  int checkForKeypress(bool *ref_flush_rx)
  {
    bool flush = 0;
    if( ref_flush_rx ) {
      flush = *ref_flush_rx;
      *ref_flush_rx = 0; //one-shot flush, for use in loops
    }
    return waitForKeypress(-1, flush, NULL); //single check for pending keypress
  }
}

