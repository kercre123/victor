#include <string.h>
#include <stdint.h>

#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_sdm.h"
#include "nrf_mbr.h"

#include "crc32.h"
#include "timer.h"
#include "recovery.h"
#include "battery.h"
#include "lights.h"
#include "hal/hardware.h"
#include "anki/cozmo/robot/spineData.h"

#include "anki/cozmo/robot/rec_protocol.h"

int main (void) {
  __disable_irq();

  // Configure our system clock
  TimerInit();
  Lights::init();

  // Power on the system
  Battery::init();
  Battery::powerOn();

  // Do recovery until our signature is okay
  EnterRecovery();
  
  Lights::stop();
  
  __enable_irq();
  sd_mbr_command_t cmd;
  cmd.command = SD_MBR_COMMAND_INIT_SD;
  sd_mbr_command(&cmd);

  sd_softdevice_vector_table_base_set(IMAGE_HEADER->vector_tbl);
  IMAGE_HEADER->entry_point();
}
