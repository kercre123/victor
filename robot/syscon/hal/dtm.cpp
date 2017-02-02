#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "ble_dtm.h"
}

#include "dtm.h"

#include "clad/robotInterface/messageEngineToRobot.h"

static bool dtm_initalized  = false;

void DTM::start(void) {
  if (dtm_initalized) {
    return ;
  }

  NVIC_DisableIRQ(RADIO_IRQn);
  NRF_TIMER0->POWER = 1;
  NRF_RADIO->POWER = 1;
  dtm_init();
  
  dtm_initalized = true;
}

void DTM::testCommand(int32_t command, int32_t freq, int32_t length, int32_t payload) {
  using namespace Anki::Cozmo::RobotInterface;

  // You cannot use DTM test modes while the process is disabled
  if (!dtm_initalized) {
    return ;
  }

  dtm_cmd(LE_RESET, 0, 0, 0);
  dtm_cmd(command, freq, length, payload);
}

void DTM::stop(void) {
  if (!dtm_initalized) {
    return ;
  }

  // Send a reset command
  dtm_cmd(LE_RESET, 0, 0, 0);

  // Power down all the devices
  NRF_RADIO->POWER = 0;
  NRF_TIMER0->POWER = 0;
  NVIC_EnableIRQ(RADIO_IRQn);

  dtm_initalized = false;
}
