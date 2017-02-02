#ifndef BATTERY_H
#define BATTERY_H

#include <stddef.h>
#include <stdint.h>

#include "clad/robotInterface/messageEngineToRobot.h"

enum CurrentChargeState {
  CHARGE_OFF_CHARGER,
  CHARGE_CHARGING,
  CHARGE_CHARGED,
  CHARGE_CHARGER_OUT_OF_SPEC
};

namespace Battery {
  // Initialize the charge pins and sensing
  void init();

  void setOperatingMode(Anki::Cozmo::BodyRadioMode mode);
  void updateOperatingMode();
  void hookButton(bool button_pressed);

  // Update the state of the battery
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
