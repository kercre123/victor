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
      case TO_CONTACTS: return Contacts::getchar(); //break;
      default: return -1; //break;
    }
  }
  
  static inline void bridge_putchar_(bridge_target_e which, char c) {
    switch(which) {
      case TO_DUT_UART: DUT_UART::putchar(c); break;
      case TO_CONTACTS: Contacts::putchar(c); Contacts::setModeRx(); /*back to rx immediately*/ break;
    }
  }
  
  void consoleBridge(bridge_target_e which, int inactivity_delay_ms, int timeout_ms)
  {
    const char exit[5] = "exit";
    int c, ei = 0;
    
    ConsolePrintf("Console Bridge <-> %s for %ims,inactive:%ims [or type 'exit']\n", which_s_(which), timeout_ms, inactivity_delay_ms);
    
    if( which == TO_CONTACTS )
      Contacts::setModeRx(); //explicit mode change required?
    
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
      }
      
      //be the bridge you were born to be
      if( (c = bridge_getchar_(which)) > -1 ) {
        if( c > 0 && c <= 0x7f ) //limit to ascii, ignore null. getline() methods usually do this.
          ConsolePutChar(c);
      }
      
      if( (c = ConsoleReadChar()) > -1 )
      {
        inactivity_delay_ms = 0; //disabled on activity
        bridge_putchar_(which, c);
        
        //watch for 'exit' cmd
        if( (c == '\r' || c == '\n') && ei == sizeof(exit)-1 ) //EOL after receiving 'exit'
          timeout_ms = 50; //wait around to hear response/echo
        else
          ei = (c == exit[ei]) ? ei+1 : 0; //index track
      }
      
    }
    
    ConsolePrintf("Console restored\n");
  }
}

