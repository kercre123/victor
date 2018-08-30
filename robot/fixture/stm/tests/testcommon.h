#ifndef TEST_COMMON_H
#define TEST_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "board.h"

enum bridge_target_e { 
  TO_DUT_UART, 
  TO_CONTACTS
};
#define BRIDGE_OPT_LOCAL_ECHO   0x1   /*echo chars from the console*/
#define BRIDGE_OPT_LINEBUFFER   0x2   /*behave as buffered console input: cached line written on enter*/
#define BRIDGE_OPT_CHG_DISABLE  0x4   /*for charge contact comms, disable 'charge while idle' behavior*/
typedef const char*(*bridge_hook_sendline_t)(const char *line, int len);

namespace TestCommon
{
  //turn on power and test for shorts (overcurrent). Exit state: powered if under ilimit, off if short/exception.
  void powerOnProtected(pwr_e net, uint16_t time_ms, int ilimit_ma, int oversample=0); //time_ms - time to monitor power draw, ilimit_ma - overcurrent threshold, 2^oversample averaging
  
  //create a console bridge for debugging test firmware
  //timeout:0=infinite. inactivity: timeout if console stays idle, opts:BRIDGE_OPT_
  //hook_sendline: in linebuffer mode, hook lets caller substitute entered line for a new one (alias?). return ascii,null-term or NULL to send original.
  void consoleBridge(bridge_target_e which, int inactivity_delay_ms=0, int timeout_ms=0, int opts = 0, bridge_hook_sendline_t hook_sendline = NULL );
  
  //block wait for any input from the console.
  //timeout[us]=0 -> no wait (check once). flush_rx=1 -> clear input buffer before wait
  //@return 0 if keypress detected. 1 if timed-out
  int waitForKeypress(uint32_t timeout=0, bool flush_rx=0, const char *printstr=0);
}

#ifdef __cplusplus
}
#endif

#endif //TEST_COMMON_H
