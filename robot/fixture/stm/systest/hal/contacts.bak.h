#ifndef __CONTACTS_H
#define __CONTACTS_H

//Charge contact comms (half-duplex uart)
namespace Contacts
{
  void init(void);
  int deinit(void); //return # of discarded chars in rx buffer (e.g. flush)
  
  //Comms, Transmit
  void commEnableTx(void); //switch to comms TX mode
  int  printf(const char * format, ... );
  void write(const char *s); //write a string to the contact uart
  void put(char c);
  
  //Comms, Receive
  void commEnableRx(void); //switch to comms RX mode
  int get(void); //get next rx'd char. -1 if rx buffer empty
  int flush(void); //flush the rx buffer. return # of discarded chars
}

#endif
