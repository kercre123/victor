#include <string.h>

extern "C" {
  #include "nrf.h"
  #include "nrf_gpio.h"
  #include "nrf_sdm.h"

  #include "softdevice_handler.h"  
}
  
#include "hardware.h"

#include "rtos.h"
#include "battery.h"
#include "motors.h"
#include "head.h"
#include "debug.h"
#include "timer.h"
#include "lights.h"
#include "tests.h"
#include "radio.h"
#include "crypto.h"
#include "bluetooth.h"

#include "boot/sha1.h"

#include "bootloader.h"

#include "anki/cozmo/robot/spineData.h"
#include "anki/cozmo/robot/rec_protocol.h"

#define SET_GREEN(v, b)  (b ? (v |= 0x00FF00) : (v &= ~0x00FF00))

__attribute((at(0x20003FFC))) static uint32_t MAGIC_LOCATION = 0;

GlobalDataToHead g_dataToHead;
GlobalDataToBody g_dataToBody;

extern void EnterRecovery(void) {
  Motors::teardown();

  MAGIC_LOCATION = SPI_ENTER_RECOVERY;
  NVIC_SystemReset();
}

extern "C" void HardFault_Handler(void) {
  // This stops the system from locking up for now completely temporary.
  NVIC_SystemReset();
}

void MotorsUpdate(void* userdata) {
	//Battery::setHeadlight(g_dataToBody.flags & BODY_FLASHLIGHT);
	//RTOS::kick(WDOG_UART);
}

static void EMERGENCY_FIX(void) {
  uint32_t* BOOLOADER_ADDR = (uint32_t*)(NRF_UICR_BASE + 0x14);
  const uint32_t ACTUAL_ADDR = 0x1F000;

  if (*BOOLOADER_ADDR != 0xFFFFFFFF) return ;
  
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Wen << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  *BOOLOADER_ADDR = ACTUAL_ADDR;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  NRF_NVMC->CONFIG = NVMC_CONFIG_WEN_Ren << NVMC_CONFIG_WEN_Pos;
  while (NRF_NVMC->READY == NVMC_READY_READY_Busy) ;
  
  NVIC_SystemReset();
}

int main(void)
{
  // Initialize the SoftDevice handler module.
  SOFTDEVICE_HANDLER_INIT(NRF_CLOCK_LFCLKSRC_SYNTH_250_PPM, NULL);

  // This makes sure the bootloader address is set, will not be in the final version
  EMERGENCY_FIX();

  // Initialize our scheduler
  RTOS::init();
  Crypto::init();

  // Setup all the things in order of priority
  Radio::init();
  Motors::init(); // NOTE: THIS CAUSES COZMO TO NOT ADVERTISE. SEEMS TO BE PPI/TIMER RELATED
  Battery::init();
  Bluetooth::init();
  Timer::init();
  Lights::init();

  Battery::powerOn();

  // We use the RNG in places during init
  //Radio::shutdown();
  //Bluetooth::advertise(); 

  Bluetooth::shutdown();
  Radio::advertise();
  
  // Let the test fixtures run, if nessessary
  #ifdef RUN_TESTS
	TestFixtures::run();
	#else
  Head::init();
	#endif

	Timer::start();

  // Run forever, because we are awesome.
  for (;;) {
    __asm { WFI };
		Crypto::manage();
  }
}
