#include <string.h>
#include "app/tests.h"
#include "hal/portable.h"
#include "hal/testport.h"
#include "hal/timers.h"
#include "hal/board.h"
#include "hal/console.h"
#include "hal/uart.h"
#include "hal/swd.h"
#include "hal/espressif.h"
#include "hal/board.h"

#include "app/app.h"
#include "app/binaries.h"
#include "app/fixture.h"
#include "hal/monitor.h"

#include "hal/motorled.h"

//Debug hack. We sometimes want to update head firmware through the face, without
//connecting the spine cable (e.g. VBAT). We can instead use a charge base powered
//by our VEXT source so we can still cycle power as needed.
#define DEBUG_SPINELESS 1

// Return true if device is detected on contacts
bool HeadDetect(void)
{
  // HORRIBLE PERMANENT HACK TIME - if we leave battery power enabled, the CPU will pull SWD high
  // Main problem is that power is always enabled, not exactly what we want
  EnableBAT();
  if( DEBUG_SPINELESS )
    EnableVEXT();
  
  // First drive SWD low for 1uS to remove any charge from the pin
  GPIO_InitTypeDef  GPIO_InitStructure;
  GPIO_InitStructure.GPIO_Pin = GPIOB_SWD;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
  PIN_RESET(GPIOB, PINB_SWD);
  GPIO_Init(GPIOB, &GPIO_InitStructure);
  MicroWait(1);
  
  // Now let it float and see if it ends up high
  PIN_IN(GPIOB, PINB_SWD);
  MicroWait(50);  // Reaches 1.72V after 25uS - so give it 50 just to be safe
   
  // Wait 1ms in detect
  MicroWait(1000);
  
  // True if high
  return !!(GPIO_READ(GPIOB) & GPIOB_SWD);
}

unsigned int serial_;
int swd_read32(int addr);

// Connect to and flash the K02
void HeadK02(void)
{ 
  const int SERIAL_ADDR = 0xFFC;
  
  // Try to talk to head on SWD
  SWDInitStub(0x20000000, 0x20001800, g_stubK02, g_stubK02End);

  // First check if our program is already on there - 0x800-0x900 (2KB) is convincing enough
  serial_ = swd_read32(SERIAL_ADDR);    // Check existing serial number
  for (int i = 0x800; i < 0x900; i += 4)
    if (*(u32*)(g_K02Boot+i) != swd_read32(i))
      serial_ = 0;                      // Do not use existing serial if there are changes
  
  // If we get this far, allocate a serial number
  ConsolePrintf("old-serial,%08x\r\n", serial_);
  if (0 == serial_ || 0xf == (serial_ >> 28))
    serial_ = GetSerial();              // Missing serial, get a new one
  ConsolePrintf("serial,%08x\r\n", serial_);
  
  // Send the bootloader and app (except on HEAD2 - you can skip the app update there)
  SWDSend(0x20001000, 0x800, 0x0,    g_K02Boot, g_K02BootEnd,   SERIAL_ADDR,  serial_);
  if (g_fixtureType != FIXTURE_HEAD2_TEST)
    SWDSend(0x20001000, 0x800, 0x1000, g_K02,     g_K02End,       0,            0);
}

static void power_off_(void)
{
  DeinitEspressif();  // XXX - would be better to ensure it was like this up-front
  SWDDeinit();
  if( DEBUG_SPINELESS )
    DisableVEXT();
  DisableBAT();     // This has a built-in delay while battery power leaches out
}

static void power_on_(bool rom_boot = true)
{
  InitEspressif(rom_boot); //init pins for ROM or APP boot + flush uart
  EnableBAT();
  if( DEBUG_SPINELESS )
    EnableVEXT();
}

// Boot K02 in test mode
void BootK02Test(void)
{
  power_off_();
  /*
  // Let head discharge (this takes a while)
  for (int i = 5; i > 0; i--)
  {
    MicroWait(1000000);
    ConsolePrintf("%d..", i);
  } 
  */
  power_on_();
}

// Connect to and flash the Espressif
void HeadESP(void)
{
  EraseEspressif();
  
  //power cycle before we can reprogram the erased pages (ESP limitation?)
  BootK02Test();
  
  // Program espressif, which will start up, following the program
  ProgramEspressif(serial_);
}

//reset and monitor status of internal self tests (performed on boot)
void HeadESPSelfTest(void)
{
  const int dat_size = 1024 * 16;       //testport.c -> m_globalBuffer[1024 * 16];
  char *dat = (char*)GetGlobalBuffer(); //grab a large static buffer from tesport
  memset(dat, '\0', dat_size);
  
  power_off_();
  ConsolePrintf("ESP Self Test, Debug Output:\r\n");
  power_on_(false); //boot into application + flush uart
  
  //Save incoming esp debug spew
  int cnt = 0;
  u32 time = getMicroCounter();
  while( getMicroCounter()-time < 500*1000 )
  {
    int c = ESPGetChar(100);
    if( cnt < dat_size-1 ) { //preserve end-of-buffer null
      if( c >= ' ' && c <= '~' ) { //filter printable ascii
        dat[cnt++] = c;
        time = getMicroCounter(); //extend wait time while we're receiving data
      } else if( c == '\n' )
        dat[cnt++] = '\0'; //null terminate for string parsing
    }
  }
  
  //process lines
  char *line = dat;
  int len = strlen(line), mem_err_cnt = 0, mem_ok = 0;
  while(len)
  {
    const char message_pass[] = "Memtest PASS";
    if( !strncmp(message_pass, line, strlen(message_pass)) )
      mem_ok = 1;
    
    const char message_err[]  = "Memory Error";
    if( !strncmp(message_err, line, strlen(message_err)) )
      mem_err_cnt++;
    
    ConsolePrintf("  %s\r\n", line);
    line += len + 1;
    len = line >= dat + dat_size ? 0 : strlen(line);
  }
  
  BootK02Test(); //power cycle, back into ROM mode (required by some other tests)
  
  ConsolePrintf("esp-memtest,ok,%d,error-cnt,%d\r\n", mem_ok, mem_err_cnt);
  if( !mem_ok || mem_err_cnt > 0 )
    throw ERROR_HEAD_ESP_MEM_TEST;
}

void HeadTest(void)
{
  // XXX: This test will be a built-in self-test in Cozmo
  // Each CPU will test its own pins for shorts/opens
}

// With 2.2K, we see most of them just under 550
const int SAFE_THRESHOLD = 599;
const int PULLUP_DETECT_THRESHOLD = 300; //if pull-up is populated, reading should never be below this

// Put head in test mode
void HeadQ1Test(void)
{
  // About half a second is long enough to get booted and into test mode
  MicroWait(500000);
  
  // Sample the voltage on HEADTRX
  PIN_ANA(GPIOB, PINB_VDD);     // XXX: Stupid hack for rev 1 boards - remove!
  PIN_ANA(GPIOC, PINC_RESET);

  const int OVERSAMPLE = 8;
  int sum = 0;
  for (int i = 0; i < (1<<OVERSAMPLE); i++)
  {
    //sum += QuickADC(ADC_VDD); // XXX: Stupid hack
    sum += QuickADC(ADC_RESET); 
  }
  sum >>= OVERSAMPLE;
  ConsolePrintf("q1-mv,%d\r\n", sum);
  if (sum > SAFE_THRESHOLD)
    throw ERROR_HEAD_Q1;
  if( sum < PULLUP_DETECT_THRESHOLD ) //required pull-up (R82=2.2k) is missing
    throw ERROR_OUT_OF_RANGE;
}

TestFunction* GetHeadTestFunctions(void)
{
  static TestFunction functions[] =
  {
    HeadK02,
    BootK02Test,
    HeadESP,
    HeadESPSelfTest,
    HeadQ1Test,
    //HeadTest,
    NULL
  };

  return functions;
}

TestFunction* GetHead2TestFunctions(void)
{
  static TestFunction functions[] =
  {
    HeadK02,
    BootK02Test,
    HeadESPSelfTest,
    HeadQ1Test,
    NULL
  };

  return functions;
}
