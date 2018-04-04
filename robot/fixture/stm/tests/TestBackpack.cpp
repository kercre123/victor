#include "app.h"
#include "board.h"
#include "bpled.h"
#include "console.h"
#include "fixture.h"
#include "meter.h"
#include "portable.h"
#include "tests.h"
#include "timer.h"

//signal map (aliases)
#define BP_MICMOSI1   DUT_MOSI
#define BP_MICMISO1   DUT_MISO
#define BP_MICMISO2   DUT_SCK
#define BP_PWR_B      DUT_TX
#define BP_VBAT_RES   DUT_RX /*~3.5k PD*/
#define BP_CAP1       DUT_TRX

bool TestBackpackDetect(void)
{
  //Board::powerOn(PWR_VBAT, 0); //no delay
  //int i_ma = Meter::getCurrentMa(PWR_VBAT,0); //~250us sample time
  //return i_ma >= 3; //XXX: tweak this
  
  return false;
}

void TestBackpackCleanup(void)
{
  //LED
  BPLED::disable(); //turns off VBAT
  
  //Mics
  BP_MICMOSI1::mode(MODE_INPUT);
  BP_MICMISO1::init(MODE_INPUT, PULL_NONE);
  BP_MICMISO2::init(MODE_INPUT, PULL_NONE);
  Board::powerOff(PWR_DUTVDD,10); //discharge mic VDD
  
  //Button
  //BPLED::disable();
  BP_VBAT_RES::init(MODE_INPUT, PULL_NONE);
  BP_PWR_B::init(MODE_INPUT, PULL_NONE);
}

void TestLeds(void)
{
  ConsolePrintf("testing backpack leds\n");
  BPLED::enable(); //init gpio and power to backpack shift register
  
  int ima = Meter::getCurrentMa(PWR_VBAT,6);
  ConsolePrintf("idle current %dmA\n", ima);
  
  for(int n=0; n < BPLED::num; n++) {
    BPLED::on(n);
    Timer::wait(250*1000);
    ima = Meter::getCurrentMa(PWR_VBAT,6);
    ConsolePrintf("%s: %imA (expected %imA)\n", BPLED::description(n), ima, -1);
    BPLED::off();
  }
  
  BPLED::disable(); //turn off power, disable gpio pins
}

const int mux_stream_len = (1 << 12);

//parameterized sample loop - inlined for optimization
#define SAMPLE_MIC_STREAM(buf,MISO,CLK)               \
{                                                     \
  for( int j=0; j < (mux_stream_len * 128); j++) {    \
    /*even buffer index is SEL=Vdd mic*/              \
    /*odd buf index is SEL=Gnd mic*/                  \
    buf[j&(mux_stream_len-1)] = MISO::read();         \
    CLK::write( j & 1 );                              \
  }                                                   \
  CLK::reset();                                       \
}

void mic_print_stream(char* stream, int MKx, bool odd_nEven )
{
  const int linesize = 128;
  
  ConsolePrintf("MK%u stream:", MKx);
  for(int i=odd_nEven ; i < mux_stream_len; i +=2 ) {
    if( ((i-odd_nEven)/2) % linesize == 0 )
      ConsolePrintf("\n");
    ConsolePrintf("%u", stream[i]);
  }
  ConsolePrintf("\n");
}

//analyze a mic bitstream (mux'd pdm from 2 mics)
static int evens,odds,oddfollow,evenfollow;
static inline void analyze_mic_stream_(char* bit, int even_MKx, int odd_MKx )
{
  evens = 0, odds = 0, oddfollow = 0, evenfollow = 0;
  
  //count '1's in each mic stream
  for(int i=0; i < mux_stream_len; i+=2) {
    if( bit[i] )
      ++evens;
  }
  for(int i=1; i < mux_stream_len; i+=2) {
    if( bit[i] )
      ++odds;
  }
  
  //count following matches (test for floating out from one mic)
  for(int i=0; i < mux_stream_len-1; i+=2) {
    if( bit[i] == bit[i+1] )
      ++oddfollow; //odd follows the preceding even
  }
  for(int i=1; i < mux_stream_len-1; i+=2) {
    if( bit[i] == bit[i+1] )
      ++evenfollow; //even follows the preceding odd
  }
}

static void TestMics(void)
{
  struct {
    int bitCnt;
    int bitPct;
    int followCnt;
    int followPct;
    bool pass;
  }mic[4];
  memset(&mic,0,sizeof(mic));
  
  //init pins
  BP_MICMOSI1::reset();
  BP_MICMOSI1::init(MODE_OUTPUT, PULL_NONE, TYPE_PUSHPULL, SPEED_HIGH);
  BP_MICMISO1::init(MODE_INPUT, PULL_DOWN);
  BP_MICMISO2::init(MODE_INPUT, PULL_UP);
  
  //turn on power
  Board::powerOn(PWR_DUTVDD,100);
  
  ConsolePrintf("sampling mics MK1,MK2\n");
    char *stream12 = (char*)&app_global_buffer[0];
    SAMPLE_MIC_STREAM(stream12,BP_MICMISO1,BP_MICMOSI1);
    analyze_mic_stream_(stream12, /*MK*/1, /*MK*/2);
    mic[0].bitCnt = evens;
    mic[1].bitCnt = odds;
    mic[0].followCnt = evenfollow;
    mic[1].followCnt = oddfollow;
  
  ConsolePrintf("sampling mics MK3,MK4\n");
    char *stream34 = (char*)&app_global_buffer[mux_stream_len];
    SAMPLE_MIC_STREAM(stream34,BP_MICMISO2,BP_MICMOSI1);
    analyze_mic_stream_(stream34, /*MK*/3, /*MK*/4);
    mic[2].bitCnt = evens;
    mic[3].bitCnt = odds;
    mic[2].followCnt = evenfollow;
    mic[3].followCnt = oddfollow;

  //cleanup
  TestBackpackCleanup(); //reset pins, pwr etc.
  
  //DEBUG:
  //mic_print_stream(stream12, /*MK*/1, 0); //even stream index
  //mic_print_stream(stream12, /*MK*/2, 1); //odd stream index
  //mic_print_stream(stream34, /*MK*/3, 0); //even stream index
  //mic_print_stream(stream34, /*MK*/4, 1); //odd stream index
  //-*/
  
  //analyze and print results
  for(int x=0; x<4; x++)
  {
    const int bitCntTotal = mux_stream_len/2;
    mic[x].bitPct = mic[x].bitCnt * 100 / bitCntTotal;
    mic[x].followPct = mic[x].followCnt * 100 / bitCntTotal;
    mic[x].pass = (mic[x].bitPct > 20 && mic[x].bitPct < 80) && 
                  (mic[x].followPct < 90);
    
    ConsolePrintf("MK%u bit-ratio 1:0 -> %i:%i (%i%%) -- follower %i/%u (%i%%) [%s]\n", x+1,
                        mic[x].bitCnt,    bitCntTotal-mic[x].bitCnt, mic[x].bitPct,
                        mic[x].followCnt, bitCntTotal,               mic[x].followPct,
                        mic[x].pass ? "ok" : "FAIL" );
  }
  
  //show extended led (detect broken)
  Board::ledOff(Board::LED_YLW);
  if( !mic[0].pass || !mic[1].pass || !mic[2].pass || !mic[3].pass )
    Board::ledOn(Board::LED_RED);
  else
    Board::ledOn(Board::LED_GREEN);
  Timer::delayMs(1000);
  
  Board::ledOff(Board::LED_GREEN);
  Board::ledOff(Board::LED_RED);
  if( !mic[0].pass || !mic[1].pass || !mic[2].pass || !mic[3].pass )
    throw ERROR_BACKP_LED;
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

void TestButton(void)
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
    //TestLeds,
    TestMics,
    //TestButton,
    NULL,
  };
  return m_tests;
}

