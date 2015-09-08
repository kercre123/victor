#include "nrf.h"
#include "nrf_gpio.h"

#include "hardware.h"
#include "drop.h"

void Drop::init() {
#ifdef ROBOT41
  // Turn off drop led
  nrf_gpio_pin_clear(PIN_IR_DROP);
  nrf_gpio_cfg_output(PIN_IR_DROP);

  // turn off headlight
  nrf_gpio_pin_clear(PIN_IR_FORWARD);
  nrf_gpio_cfg_output(PIN_IR_FORWARD);
#endif
}
