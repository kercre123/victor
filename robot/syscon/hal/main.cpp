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

#include "sha1.h"

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

static const uint8_t secret1[SECRET_LENGTH] = {0,0,0,0,0,0,0,0,0,0};
static const uint8_t secret2[SECRET_LENGTH] = {0,0,0,0,0,0,0,0,0,0};
static CryptoTask y;
static DiffieHellman dh;
static uint8_t dh_key[AES_KEY_LENGTH];

static void DH_Phase1(CryptoTask* task) {
	DiffieHellman* dh = (DiffieHellman*) task->input;
	memcpy(dh->secret, secret2, sizeof(secret2));
}

static void DH_Phase2(CryptoTask* task) {
	DiffieHellman* dh = (DiffieHellman*) task->input;
}

static void TestCrypto(void) {
	CryptoTask x;
	x.op = CRYPTO_START_DIFFIE_HELLMAN;
	x.input = &dh;
	x.callback = DH_Phase1;
	Crypto::execute(&x);

	CryptoTask y;
	y.op = CRYPTO_FINISH_DIFFIE_HELLMAN;
	y.input = &dh;
	y.output = &dh_key;
	y.length = sizeof(dh_key);
	y.callback = DH_Phase2;
	Crypto::execute(&y);
	
	dh.mont = &DEFAULT_DIFFIE_GROUP;
	dh.gen = &DEFAULT_DIFFIE_GENERATOR;
	dh.pin = 9999;
	memcpy(dh.secret, secret1, sizeof(secret1));
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

  Battery::powerOn();
  
  // Setup all tasks
  Battery::init();
  Bluetooth::init();
  Timer::init();
  Lights::init();
  Radio::init();

  #ifndef BLUETOOTH_MODE
  Motors::init(); // NOTE: THIS CAUSES COZMO TO NOT ADVERTISE. SEEMS TO BE PPI/TIMER RELATED

  Bluetooth::shutdown();
  Radio::advertise();
  #else
  Radio::shutdown();
  Bluetooth::advertise(); 
  #endif
  
  // Let the test fixtures run, if nessessary
  #ifdef RUN_TESTS
  TestFixtures::run();
  #else
  Head::init();
  #endif

  //Timer::start();

	TestCrypto();

  // Run forever, because we are awesome.
  for (;;) {
    __asm { WFI };
    Crypto::manage();
  }
}
