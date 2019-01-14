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

typedef struct
{
  //in linebuffer mode, reports the entered line prior to sending.
  //return param allows line substitutions (alias, debug etc)
  //@return NULL = send reported line, unmodified.
  //        null-terminated ascii string to substitute.
  const char*(*pre_send)(const char *line, int len);
  
  //in linebuffer mode, reports a line after being sent
  void(*post_send)(const char *line, int len);
  
  //periodic timer tick
  //timeout_remain_ms - remaining timeout (0=disabled)
  //inactivity_remain_ms - remaining inactivity timer (0=disabled)
  //idle_ms - bridge idle timer (reset to 0 on activity)
  void(*tick_ms)(int timeout_remain_ms, int inactivity_remain_ms, int idle_ms);
  
} bridge_hooks_t;

namespace TestCommon
{
  //turn on power and test for shorts (overcurrent). Exit state: powered if under ilimit, off if short/exception.
  void powerOnProtected(pwr_e net, uint16_t time_ms, int ilimit_ma, int oversample=0); //time_ms - time to monitor power draw, ilimit_ma - overcurrent threshold, 2^oversample averaging
  
  //create a console bridge for debugging test firmware
  //timeout:0=infinite. inactivity: timeout if console stays idle, opts:BRIDGE_OPT_
  void consoleBridge(bridge_target_e which, int inactivity_delay_ms=0, int timeout_ms=0, int opts=0, bridge_hooks_t *hooks=NULL );
  
  //block wait for any input from the console.
  //timeout[us]: 0=infinite, <0 no wait (checks once)
  //flush_rx: 1 = clear input buffer before wait/check
  //printstr: optional console print before wait begins
  //@return 0 if keypress detected. 1 if timed-out
  int waitForKeypress(int timeout=0, bool flush_rx=0, const char *printstr=0);
  
  //non-block check for keypress (loop usage)
  //ref_flush_rx: loop bool. cleared after each check. use for one-shot buffer flush 1st loop pass
  int checkForKeypress(bool *ref_flush_rx=0);
}

#ifdef __cplusplus
}
#endif

#endif //TEST_COMMON_H
