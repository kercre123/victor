extern "C" {
  #include "nrf.h"
}

#include "dtm.h"

#include "clad/robotInterface/messageEngineToRobot.h"

static bool dtm_initalized  = false;

void DTM::start(void) {
  if (dtm_initalized) {
    return ;
  }

  NRF_RADIO->POWER = 1;
  
  dtm_initalized = true;
}

void DTM::enterTestMode(uint8_t mode, uint8_t channel) {
  using namespace Anki::Cozmo::RobotInterface;

  // You cannot use DTM test modes while the process is disabled
  if (!dtm_initalized) {
    return ;
  }

  /*
  NRF_RADIO->TASKS_DISABLE = 1;
  
  switch(mode) {
    case DTM_DISABLED:
      break ;
    case DTM_LISTEN:
      NRF_RADIO->TASKS_RXEN = 1;
      break ;
    case DTM_CARRIER:
      NRF_RADIO->TASKS_TXEN = 1;
      break ;
    case DTM_TRANSMIT:
      NRF_RADIO->TASKS_TXEN = 1;
      break ;
  }
  */
}

void DTM::stop(void) {
  if (!dtm_initalized) {
    return ;
  }
  
  NRF_RADIO->POWER = 0;
  
  dtm_initalized = false;
}
