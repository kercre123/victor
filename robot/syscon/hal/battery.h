#ifndef BATTERY_H
#define BATTERY_H

#include <stddef.h>
#include <stdint.h>

#include "clad/robotInterface/messageEngineToRobot.h"

namespace Battery {
  // Initialize the charge pins and sensing
  void init();

  void setOperatingMode(Anki::Cozmo::BodyRadioMode mode);
  void updateOperatingMode();

  // Update the state of the battery
  void setHeadlight(bool status);
  void manage(void);
  uint8_t getLevel(void);

  // Turn on power to the head
  void powerOn();

  // Completely disable the machine
  void powerOff();

  // Are we on the contacts right now?
  #define IsOnContacts() m_onContacts

  extern bool onContacts;
}

#endif
