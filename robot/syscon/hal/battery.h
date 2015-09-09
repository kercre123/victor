#ifndef BATTERY_H
#define BATTERY_H

namespace Battery {
  // Initialize the charge pins and sensing
  void init();

  // Update the state of the battery
  void update();

  // Turn on power to the head
  void powerOn();

  // Completely disable the machine
  void powerOff();

  // Are we on the contacts right now?
  #define IsOnContacts() m_onContacts

  extern bool onContacts;
}

#endif
