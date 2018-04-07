#ifndef __CONTACTS_H
#define __CONTACTS_H

//Charge contacts: Power + half-duplex uart
namespace Contacts
{
  void init(void);
  void deinit(void);
  void echo(bool on); //if supported, turns on console-echo
  void setModeIdle(void); //disconnect pins etc
  
  //Power control
  void setModePower(void); //switch to power mode (automatic on any power API call)
  void powerOn(int turn_on_delay_ms = 10);   //VEXT->5V. Wait 'turn_on_delay_ms' for power to stabilize (delay ignored if power is on)
  void powerOff(int turn_off_delay_ms = 50, bool force = 1); //VEXT->high-Z. Wait 'turn_off_delay_ms' for power to drain out (delay ignored if power is off). if force=1, drive VEXT low to discharge quickly.
  bool powerIsOn(void); //true if power API enabled and power is ON
  
  //Transmit
  void setModeTx(void); //switch to uart TX mode (automatic on any tx API call)
  int  printf(const char * format, ... );
  void write(const char *s);
  void putchar(char c);
  //void flushTx(void); //return after all outstanding chars are sent out the TX pin
  
  //Receive
  void setModeRx(void); //switch to uart RX mode (automatic on any rx API call)
  int  getchar(void); //get next rx'd char. -1 if rx buffer empty
  char* getline(int timeout_us = 0, int *out_len = 0); //+manages console features, if supported
  char* getlinebuffer(int *out_len = 0); //access to the raw line buffer (debug: access to partial rx)
  int  flushRx(void); //flush the rx buffer (+line buffer). return # of discarded chars
  int getRxDroppedChars();   //get and clear count of dropped rx chars
  int getRxOverflowErrors(); //get and clear count of receive buffer overflows (uart periph)
  int getRxFramingErrors();  //get and clear count of receive framing errors
}

#endif
