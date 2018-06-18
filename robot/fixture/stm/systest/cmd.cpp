#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "board.h"
#include "cmd.h"
#include "contacts.h"
#include "i2c.h"
#include "opto.h"
#include "timer.h"

FailureCode Opto::failure = BOOT_FAIL_NONE;

//picked from from 'robotcom.h'
//robot sensor index for 'mot' + 'get' cmds
#define RCOM_SENSOR_NONE        0
#define RCOM_SENSOR_BATTERY     1
#define RCOM_SENSOR_CLIFF       2
#define RCOM_SENSOR_MOT_LEFT    3
#define RCOM_SENSOR_MOT_RIGHT   4
#define RCOM_SENSOR_MOT_LIFT    5
#define RCOM_SENSOR_MOT_HEAD    6
#define RCOM_SENSOR_PROX_TOF    7
#define RCOM_SENSOR_BTN_TOUCH   8
#define RCOM_SENSOR_RSSI        9
#define RCOM_SENSOR_RX_PKT      10
#define RCOM_SENSOR_DEBUG_INC   11

//-----------------------------------------------------------
//        Line Parsing (hack, src copy from cube console)
//-----------------------------------------------------------
//Note: valid command strings must guarantee no \r \n chars and one valid \0 terminator

static inline bool is_whitespace_(char c) {
  return c <= ' ' || c > '~'; //space or non-printable char
}

//seek to start of next argument.
static char* next_arg_(char* s)
{
  //started on a quoted arg. seek to end of quotes
  bool inQuotes = *s == '"';
  while( inQuotes ) {
    if( *++s == NULL )
      return NULL;
    if( *s == '"' )
      break;
  }
  
  //started on normal arg (or closing quote from above). seek next whitespace
  while( !is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  //finally, seek start of next arg
  while( is_whitespace_(*s) ) {
    if( *++s == NULL )
      return NULL;
  }
  
  return s; //now pointing to first char of next arg (maybe an opening ")
}

char* console_getarg(char *s, int n, char* buf, int buflen)
{
  if( !s || !buf )
    return NULL;
  
  if( is_whitespace_(*s) ) //need initial seek to start from 0th arg
    n++;
  while(n--) { //seek to nth arg
    if( (s = next_arg_(s)) == NULL )
      return NULL; //there is no spoon
  }
  
  //find arg length
  int len = 0;
  bool inQuotes = *s == '"';
  if( inQuotes ) {
    char* temp = ++s;
    while(1) {
      if(*temp == '"' || *temp == NULL)
        break;
      len++; temp++;
    }
  } 
  else { //normal arg
    char* temp = s;
    do { 
      len++;
    } while(*++temp != NULL && !is_whitespace_(*temp) );
  }
  
  //copy to output buffer
  len = len > buflen-1 ? buflen-1 : len; //limit to buffer size
  memcpy(buf, s, len );
  buf[len] = '\0';
  return buf;
}

char* console_getargl(char *s, int n)
{
  const int bufsize = 39;
  static char buf[bufsize+1];
  return console_getarg(s, n, buf, sizeof(buf));
}

int console_num_args(char *s)
{
  if(!s)
    return 0;
  
  int n = is_whitespace_(*s) ? 0 : 1;
  while( (s = next_arg_(s)) != NULL )
    n++;
  return n;
}

//-----------------------------------------------------------
//        Output
//-----------------------------------------------------------

static void writes_(const char *s) { 
  if(s) Contacts::write(s);
}

static int respond_(char* cmd, int status, const char* info)
{
  char b[10]; int bz = sizeof(b);
  
  //<<cmd # [info]
  writes_(RSP_PREFIX);
  writes_(cmd);
  writes_( snformat(b,bz," %i ",status) );
  writes_(info);
  writes_("\n");
  
  return status;
}

//-----------------------------------------------------------
//        Commands
//-----------------------------------------------------------

int cmd_process(char* s)
{
  char b[50]; int bz = sizeof(b);
  
  if(!s)
    return STATUS_NULL;
  
  //is this a command?
  int len = strlen(s);
  if( !strncmp(s, CMD_PREFIX, sizeof(CMD_PREFIX)-1) && len > sizeof(CMD_PREFIX)-1 ) //valid cmd formatting
    s += sizeof(CMD_PREFIX)-1, len -= sizeof(CMD_PREFIX)-1; //skip the prefix for parsing
  else
    return STATUS_NOT_A_CMD;
  
  //copy command arg into local buffer
  char cmd_buf[20], *cmd = console_getarg(s, 0, cmd_buf, sizeof(cmd_buf));
  
  //================
  //get version
  //>>getvers
  //<<getvers [status] [0xversion]
  //================
  if( !strcmp(cmd, "getvers") ) {
    writes_("project = systest\n");
    writes_(__DATE__ " " __TIME__ "\n");
    return respond_(cmd, STATUS_OK, snformat(b,bz,"hw=%s",HWVERS) );
  }//-*/
  
  //==========================
  //Set LEDs
  //>>leds [bitfield<n>]
  //<<leds [status]
  //==========================
  if( !strcmp(cmd, "leds") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int ledbf = strtol(console_getargl(s,1), 0,0); //hex ok - must have 0x prefix for cstd parser
    //XXX: hook into backpack driver?
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, "0x%x", ledbf) );
  }//-*/
  
  //==========================
  //Delay (for testing)
  //>>delay [#ms]
  //<<delay 0
  //==========================
  if( !strcmp(cmd, "delay") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int ms = strtol(console_getargl(s,1), 0,0); //hex ok - must have 0x prefix for cstd parser
    writes_( snformat(b,bz,"spin for %ims\n", ms) );
    
    while(ms--) {
      Timer::wait(1000);
      if( ms > 0 && ms%1000 == 0)
        writes_( snformat(b,bz,"%us\n",ms/1000) );
    }
    
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  //==========================
  //Console Echo
  //>>echo {off/on}
  //==========================
  if( !strcmp(cmd, "echo") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int echo = -1;
    char* arg1 = console_getargl(s,1);
    if( !strcmp(arg1, "off") )
      Contacts::echo( echo=0 );
    else if( !strcmp(arg1, "on") )
      Contacts::echo( echo=1 );
    
    if( echo > -1 )
      return respond_(cmd, STATUS_OK, arg1);
    else
      return respond_(cmd, STATUS_ARG_PARSE, "bad arg");
  }//-*/
  
  //==========================
  //>>vdd {0,1}
  //==========================
  if( !strcmp(cmd, "vdd") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    //since this will probably shut off the mcu, respond before enacting
    int on = strtol(console_getargl(s,1), 0,0);
    int rsp = respond_(cmd, STATUS_OK, snformat(b,bz, on ? "on" : "off") );
    
    Board::pwr_vdd( on > 0 );
    return rsp;
  }//-*/
  
  //==========================
  //>>vdds {0,1}
  //==========================
  if( !strcmp(cmd, "vdds") )
  {
    if( console_num_args(s) < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int on = strtol(console_getargl(s,1), 0,0);
    Board::pwr_vdds( on > 0 );
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, on ? "on" : "off") );
  }//-*/
  
  //==========================
  //>>vmain {0,1}
  //==========================
  if( !strcmp(cmd, "vmain") )
  {
    int nargs = console_num_args(s);
    if( nargs < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int on = strtol(console_getargl(s,1), 0,0);
    Board::pwr_vmain( on > 0 );
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, on ? "on" : "off") );
  }//-*/
  
  //==========================
  //>>charge power {0,1}
  //==========================
  if( !strcmp(cmd, "charger") )
  {
    int nargs = console_num_args(s);
    if( nargs < 2 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int on = strtol(console_getargl(s,1), 0,0);
    Board::pwr_charge( on > 0 );
    
    return respond_(cmd, STATUS_OK, snformat(b,bz, on ? "on" : "off") );
  }//-*/
  
  //==========================
  //Test Tread Encoders
  //==========================
  if( !strcmp(cmd, "encoders") )
  {
    struct { int on; int off; } rtenc, ltenc;
    
    //test encoder on signal
    RTENC::init(MODE_INPUT, PULL_DOWN);
    LTENC::init(MODE_INPUT, PULL_DOWN);
    Board::pwr_vdds(1);
    Timer::wait(5000);
    rtenc.on = RTENC::read();
    ltenc.on = LTENC::read();
    
    //test encoder off signal
    RTENC::init(MODE_INPUT, PULL_UP);
    LTENC::init(MODE_INPUT, PULL_UP);
    Board::pwr_vdds(0);
    Timer::wait(5000);
    rtenc.off = RTENC::read();
    ltenc.off = LTENC::read();
    
    //pin reset
    RTENC::init(MODE_INPUT, PULL_NONE);
    LTENC::init(MODE_INPUT, PULL_NONE);
    
    bool rtok = rtenc.on==1 && rtenc.off==0;
    bool ltok = ltenc.on==1 && ltenc.off==0;
    writes_( snformat(b,bz,"right tread on,off %i,%i %s\n", rtenc.on, rtenc.off, rtok?"":"--FAIL") );
    writes_( snformat(b,bz,"left  tread on,off %i,%i %s\n", ltenc.on, ltenc.off, ltok?"":"--FAIL") );
    
    if( !rtok )
      return respond_(cmd, ERROR_BODY_TREAD_ENC_RIGHT, 0);
    if( !ltok )
      return respond_(cmd, ERROR_BODY_TREAD_ENC_LEFT, 0);
    
    return respond_(cmd, STATUS_OK, 0);
  }//-*/
  
  //==========================
  //Sensor Read (using charge comm protocol)
  //>>get NN sr 00 00 00 00
  //:data [data] [data] [data]
  //<<get 0
  //==========================
  if( !strcmp(cmd, "get") )
  {
    int nargs = console_num_args(s);
    if( nargs < 3 )
      return respond_(cmd, STATUS_ARG_NA, "missing-args");
    
    int NN = strtol(console_getargl(s,1), 0,0);
    int sr = strtol(console_getargl(s,2), 0,0);
    if( NN<1 || NN>255 )
      return respond_(cmd, STATUS_ARG_NA, "NN-invalid");
    
    //-------------------------------------
    //Get Battery [:batAdc temp]
    //-------------------------------------
    if( sr == RCOM_SENSOR_BATTERY )
    {
      #define VIN_RAW_TO_MV(raw)     (((raw)*2800)>>11)  /*robot_sr_t::bat.raw (adc) to millivolts*/
      
      /*static bool adc_init = 0;
      if( !adc_init ) {
        adc_init=1;
        writes_( snformat(b,bz,"init adc\n") ); 
        
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE); //VIN_SENSE=A4
        
        ADC_InitTypeDef adcinit;
        memset(&adcinit, 0, sizeof(adcinit));
        adcinit.ADC_Resolution = ADC_Resolution_12b;
        adcinit.ADC_ContinuousConvMode = DISABLE;
        adcinit.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
        adcinit.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T1_TRGO;
        adcinit.ADC_DataAlign = ADC_DataAlign_Right; //ADC_DataAlign_Left
        adcinit.ADC_ScanDirection = ADC_ScanDirection_Upward;
        ADC_Init(ADC1, &adcinit);
      }*/
      
      VIN_SENSE::init(MODE_ANALOG);
      Timer::wait(100);
      
      //XXX
      writes_( snformat(b,bz,"WARNING: adc disabled. reporting dummy data for debug\n") );
      
      writes_( snformat(b,bz,"battery(%i) rawADC temp\n", NN) );
      for(int n=0; n<NN; n++)
      {
        //ADC_StartOfConversion(ADC1);
        //uint16_t vin_raw = ADC_GetConversionValue(ADC1);
        uint16_t vin_raw = 2816 + n;
        
        writes_( snformat(b,bz,":%i %i %i\n", vin_raw, 0, VIN_RAW_TO_MV(vin_raw)) );
        Timer::delayMs(5);
      }
      
      VIN_SENSE::init(MODE_INPUT);
      
      /*writes_( snformat(b,bz,"disable adc\n") );
      ADC_DeInit(ADC1);
      adc_init = 0;*/
      
      return respond_(cmd, STATUS_OK, 0);
    }
    
    //-------------------------------------
    //Get Cliff [:fL fR bL bR]
    //-------------------------------------
    else if( sr == RCOM_SENSOR_CLIFF )
    {
      #define TARGET(value) sizeof(value), (void*)&value
      #define VALUE(value) 1, (void*)value
      uint16_t cliffSense[4];
      
      /*static bool i2c_init = 0;
      if( !i2c_init ) {
        i2c_init=1;
        writes_( snformat(b,bz,"init i2c\n") );
        I2C::init();
      }*/
      
      //I2C::capture();
      {
        Opto::failure = BOOT_FAIL_NONE;
        const I2C_Operation I2C_LOOP[] = {
          { I2C_REG_READ, 0, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[3]) },
          { I2C_REG_READ, 1, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[2]) },
          { I2C_REG_READ, 2, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[1]) },
          { I2C_REG_READ, 3, DROP_SENSOR_ADDRESS, PS_DATA_0, TARGET(cliffSense[0]) },
          { I2C_DONE },
        };
        
        /*writes_( snformat(b,bz,"init drop sensors\n") );
        for (int i = 0; i < 4; i++) {
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, MAIN_CTRL, 0x01);
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, PS_LED, 6 | (5 << 4));
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, PS_PULSES, 8);
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, PS_MEAS_RATE, 3 | (3 << 3) | 0x40);
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_0, 0);
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, PS_CAN_1, 0);
        }*/
        
        //XXX
        writes_( snformat(b,bz,"WARNING: cliff sensors disabled. reporting dummy data for debug\n") );
        
        writes_( snformat(b,bz,"cliff(%i) fL fR bL bR\n", NN) );
        for(int n=0; n<NN; n++) {
          //I2C::execute(I2C_LOOP); //read sensors
          for(int x=0; x<4; x++) cliffSense[x] = 1234+(4*n)+x;
          
          writes_( snformat(b,bz,":%i %i %i %i\n", cliffSense[0], cliffSense[1], cliffSense[2], cliffSense[3]) );
          Timer::delayMs(5);
        }
        
        /*writes_( snformat(b,bz,"disable drop sensors\n") );
        for (int i = 0; i < 4; i++) {
          I2C::writeReg(i, DROP_SENSOR_ADDRESS, MAIN_CTRL, 0x00);
        }*/
      }
      //I2C::release();
      
      //XXX: this doesn't exist yet
      //writes_( snformat(b,bz,"teardown i2c\n") );
      //I2C::deinit();
      //i2c_init=1;
      
      return respond_(cmd, STATUS_OK, 0);
    }
    
    //-------------------------------------
    //(unsupported sr type)
    //-------------------------------------
    else
      return respond_(cmd, STATUS_ARG_NA, "unsupported-sensor");
  }
  
  //==========================
  //Test Motor FETs for shorts
  //==========================
  if( !strcmp(cmd, "motorshort") )
  {
    return respond_(cmd, STATUS_UNKNOWN_CMD, "XXX implement me");
  }
  
  //==========================
  //Unknown (catch-all)
  //==========================
  return respond_(cmd, STATUS_UNKNOWN_CMD, "unknown");
}

