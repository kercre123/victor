#include "board.h"
#include "bpled.h"
#include "console.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "tests.h"
#include "timer.h"

//signal map (aliases)
#define BP_LED_DAT    DUT_SWD
#define BP_LED_CLK    DUT_SWC
#define BP_MICMOSI1   DUT_MOSI
#define BP_MICMISO1   DUT_MISO
#define BP_MICMISO2   DUT_SCK
#define BP_PWR_B      DUT_RX
#define BP_VBAT_RES   DUT_TX
#define BP_VDD        DUT_VDD
#define BP_CAP1       DUT_TRX

bool TestBackpackDetect(void)
{
  //#warning "DEBUG: enable manual backpack testing"
  //return true;
  
  // Make sure power is not applied, as it messes up the detection code below
  Board::disableVBAT();
  BP_VDD::init(MODE_INPUT, PULL_NONE);
  
  //weakly pulled-up - it will detect as grounded when the board is attached
  BP_LED_DAT::init(MODE_INPUT, PULL_UP);
  Timer::wait(100);
  bool detected = !BP_LED_DAT::read(); //true if TRX is pulled down by body board
  BP_LED_DAT::init(MODE_INPUT, PULL_NONE); //prevent pu from doing strange things (phantom power)
  
  return detected;
}

void TestBackpackCleanup(void)
{
  //LED
  Board::disableVBAT();
  BPLED::disable();
  
  //Mics
  BP_MICMOSI1::mode(MODE_INPUT);
  BP_VDD::reset(); //discharge mic VDD
  Timer::wait(10*1000);
  BP_VDD::mode(MODE_INPUT);
  
  //Button
  //BPLED::disable();
  BP_VBAT_RES::init(MODE_INPUT, PULL_NONE);
  BP_PWR_B::init(MODE_INPUT, PULL_NONE);
}

static void TestLeds(void)
{
  ConsolePrintf("testing backpack leds\n");
  BPLED::enable(); //init gpio and power to backpack shift register
  
  int ima = Meter::getVbatCurrentMa(6);
  ConsolePrintf("idle current %dmA\n", ima);
  
  for(int n=0; n < BPLED::num; n++) {
    BPLED::on(n);
    Timer::wait(330*1000);
    ima = Meter::getVbatCurrentMa(6);
    ConsolePrintf("%s: %imA (expected %imA)\n", BPLED::description(n), ima, -1);
    BPLED::off();
  }
  
  BPLED::disable(); //turn off power, disable gpio pins
}

//parameterized sample loop - inlined for optimization
#define SAMPLE_MIC_STREAM(buf,MISO,CLK)   \
{                                         \
  for( int i=0; i < 1024 * 256; i++) {    \
    /*even buffer index is SEL=Vdd mic*/  \
    /*odd buf index is SEL=Gnd mic*/      \
    buf[i&1023] = MISO::read();           \
    CLK::write( i & 1 );                  \
  }                                       \
  CLK::reset();                           \
}

//analyze a mic bitstream (mux'd pdm from 2 mics)
static inline void analyze_mic_stream_(char* bits, int even_MKx, int odd_MKx )
{
  int evens = 0, odds = 0;
  
  //count '1's in each mic stream
  for(int i=0; i < 1024; i+=2) {
    if( bits[i] )
      ++evens;
  }
  for(int i=1; i < 1024; i+=2) {
    if( bits[i] )
      ++odds;
  }
  
  //show some stats
  ConsolePrintf("MK%u 1:0 %u:%u (%u%%)\n", even_MKx, evens, 512-evens, evens*100/512 );
  ConsolePrintf("MK%u 1:0 %u:%u (%u%%)\n", odd_MKx , odds , 512-odds , odds *100/512 );
  
  //DEBUG:
  if( 1 )
  {
    const int linesize = 64;
    
    ConsolePrintf("MK%u stream:", even_MKx );
    for(int i=0; i < 1024; i+=2) {
      if( ((i-0)/2) % linesize == 0 )
        ConsolePrintf("\n");
      ConsolePrintf("%u", bits[i]);
    }
    ConsolePrintf("\n");
    
    ConsolePrintf("MK%u stream:", odd_MKx );
    for(int i=1; i < 1024; i+=2) {
      if( ((i-1)/2) % linesize == 0 )
        ConsolePrintf("\n");
      ConsolePrintf("%u", bits[i]);
    }
    ConsolePrintf("\n");
  }
}

static void TestMics(void)
{
  //init pins
  BP_MICMOSI1::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
  BP_MICMOSI1::reset();
  BP_MICMISO1::init(MODE_INPUT, PULL_DOWN);
  BP_MICMISO2::init(MODE_INPUT, PULL_UP);
  
  //turn on power
  BP_VDD::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_LOW);
  BP_VDD::set();
  Timer::wait(100*1000);
  
  ConsolePrintf("sampling mics MK1,MK2\n");
  {
    char bits[1024];
    SAMPLE_MIC_STREAM(bits,BP_MICMISO1,BP_MICMOSI1);
    analyze_mic_stream_(bits, /*MK*/1, /*MK*/2);
  }
  
  ConsolePrintf("sampling mics MK3,MK4\n");
  {
    char bits[1024];
    SAMPLE_MIC_STREAM(bits,BP_MICMISO2,BP_MICMOSI1);
    analyze_mic_stream_(bits, /*MK*/3, /*MK*/4);
  }
  
  //cleanup
  BP_MICMOSI1::mode(MODE_INPUT);
  BP_VDD::reset(); //discharge mic VDD
  Timer::wait(10*1000);
  BP_VDD::mode(MODE_INPUT);
}

//display pattern for button test
static const uint32_t led_frame_[] = {
  0,
  BF_D1_RED   | BF_D2_RED   | BF_D3_RED,
  0,
  BF_D1_GRN   | BF_D2_GRN   | BF_D3_GRN,
  0,
  BF_D1_BLU   | BF_D2_BLU   | BF_D3_BLU,
};
static const int LED_FRAME_COUNT = sizeof(led_frame_)/sizeof(uint32_t);

static void led_manage_(bool reset = false);
static void led_manage_(bool reset)
{
  static int frame_time = 0, frame_idx = 0;
  if( reset ) {
    BPLED::off();
    frame_time = 0;
    frame_idx = 0;
    return;
  }
  
  //Cycle through LED states every Xms
  if( !frame_time || Timer::get() - frame_time > 250*1000 ) {
    frame_time = Timer::get();
    frame_idx = (frame_idx + 1) % LED_FRAME_COUNT;
  }
  
  BPLED::putFrame( led_frame_[frame_idx] ); //drive 1 display cycle every update
}

static void btn_execute_(int *press_cnt, int *release_cnt)
{
  //read button
  bool pressed = BP_VBAT_RES::read();
  
  //increment press/release counters
  if(press_cnt != NULL)
    *press_cnt = !pressed ? 0 : (*press_cnt >= 65535 ? 65535 : *press_cnt+1);
  if(release_cnt != NULL)
    *release_cnt = pressed ? 0 : (*release_cnt >= 65535 ? 65535 : *release_cnt+1);
}

static void TestButton(void)
{
  uint32_t btn_start, cnt_compare = 50; //adjust LPF count for different sample timing
  
  //init gpio
  BPLED::enable(); //use leds to display blink pattern
  BP_VBAT_RES::init(MODE_INPUT, PULL_DOWN);
  BP_PWR_B::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL);
  BP_PWR_B::set();
  
  led_manage_(1); //reset
  
  ConsolePrintf("Waiting for button...\n");
  
  //wait for button press
  int btn_press_cnt = 0;
  btn_start = Timer::get();
  while( btn_press_cnt < cnt_compare ) {
    btn_execute_( &btn_press_cnt, 0 ); //monitor button state & manage leds
    led_manage_();
    if( Timer::get() - btn_start > 5*1000*1000 ) {
      //throw ERROR_BACKPACK_BTN_PRESS_TIMEOUT;
      ConsolePrintf("ERROR_BACKPACK_BTN_PRESS_TIMEOUT\n");
      break;
    }
  }
  
  ConsolePrintf("btn pressed\n");
  
  //wait for button release
  int btn_release_cnt = 0;
  btn_start = Timer::get();
  while( btn_release_cnt < cnt_compare ) {
    btn_execute_( 0, &btn_release_cnt );
    led_manage_();
    if( Timer::get() - btn_start > 3500*1000 ) {
      //throw ERROR_BACKPACK_BTN_RELEASE_TIMEOUT;
      ConsolePrintf("ERROR_BACKPACK_BTN_RELEASE_TIMEOUT\n");
      break;
    }
  }
  
  ConsolePrintf("btn released\n");
  
  BPLED::disable();
  BP_VBAT_RES::init(MODE_INPUT, PULL_NONE);
  BP_PWR_B::init(MODE_INPUT, PULL_NONE);
}

TestFunction* TestBackpack1GetTests(void)
{
  static TestFunction m_tests[] = {
    TestLeds,
    TestMics,
    TestButton,
    NULL,
  };
  return m_tests;
}

