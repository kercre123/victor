extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"
}

#include "temp.h"
#include "bluetooth.h"

int32_t temperature;

void Temp::init(void) {
  // Enable temprature sensor
  NRF_TEMP->POWER = 1;
  NRF_TEMP->TASKS_START = 1;
}

void Temp::manage(void) {
  if (Bluetooth::enabled()) {
    sd_temp_get(&temperature);
  } else {
    if (NRF_TEMP->EVENTS_DATARDY) {
      temperature = NRF_TEMP->TEMP;
      NRF_TEMP->EVENTS_DATARDY = 0;
    }
    
    NRF_TEMP->TASKS_START = 1;
  }
}

int32_t Temp::getTemp(void) {
  return temperature;
}
