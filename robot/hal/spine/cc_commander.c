#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>

enum {
  ERR_PENDING = -1, //does not return right away
  ERR_OK = 0,
  ERR_UNKNOWN,
  ERR_SYNTAX,
  ERR_SYSTEM,
  ERR_LOCKED    //packout
};

#ifdef STANDALONE_UTILITY
#include "core/common.h"
#include "core/lcd.h"
#include "helpware/kbhit.h"
#include "helpware/display.h"
#endif

#include "spine_hal.h"
#include "schema/messages.h"

#define ccc_debug(fmt, args...)  (LOGD( fmt, ##args))

#define EXTENDED_CCC_DEBUG 0
#if EXTENDED_CCC_DEBUG
#define ccc_debug_x ccc_debug
#else
#define ccc_debug_x(fmt, args...)
#endif


//fixed command line is NAM A0 A1 A2 A3 A4 A5
//total 3 length chars for name + 6*3 chars for space+arg
#define FIXED_NAME_LEN 3
#define FIXED_ARG_LEN 2
#define FIXED_ARG_COUNT 6
#define FIXED_LINE_LEN FIXED_NAME_LEN+(FIXED_ARG_LEN+1)*FIXED_ARG_COUNT

#define SLUG_PAD_CHAR 0xFF
#define SLUG_PAD_SIZE 2


#define MAX_KNOWN_LOG  16
#define RESPONSE_CHUNK_SZ  (sizeof(struct ContactData)-SLUG_PAD_SIZE)

#define FACTORY_FILENAME_TEMPLATE "/factory/log%d"
#define MAX_FACTORY_FILENAME_LEN  sizeof(FACTORY_FILENAME_TEMPLATE)

#define BSV_LINES 6

static int gLogFd = 0;


typedef void (*ShutdownFunction)(int);
static int gShutdownCountdown = 0;

const struct SpineMessageHeader* hal_read_frame();
void start_overrride(int count);
extern void request_version(void);
void hal_terminate(void);


enum {
  show_NONE,
  show_BAT,
  show_CLIFF,
  show_MOTOR0,
  show_MOTOR1,
  show_MOTOR2,
  show_MOTOR3,
  show_PROX,
  show_TOUCH,
  show_RSSI,
  show_PACKETS,
  show_DEBUG_COUNTER,
  show_LAST_SENSOR,
  show_LOGS,
  show_BSV,
  show_COUNT

};

enum  {
  runstate_NORMAL,
  runstate_CMDLINE_PENDING,
  runstate_CMDLINE_ACTIVE,
} gRunState = runstate_NORMAL;


struct HeadToBody gHeadData = {0};

typedef struct CozmoCommand_t {
  uint16_t repeat_count;
  int16_t motorValues[MOTOR_COUNT];
  uint16_t printmask;
  char cmd[4];
  int result;
  char holdToken;
} CozmoCommand;


#ifdef STANDALONE_TEST
#define print_response printf
#else
int print_response(const char* format, ...);
#endif


/************* CIRCULAR BUFFER for commands ***************/

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef uint_fast16_t idx_t;
#define CMDBUF_CAPACITY (1<<8)
#define CMDBUF_SIZE_MASK (CMDBUF_CAPACITY-1)

static struct command_buffer_t {
  CozmoCommand buffer[CMDBUF_CAPACITY];
  idx_t head;
  idx_t tail;
} gCommandList = {{{0}},0,0};

static inline idx_t command_buffer_count() {
  return ((gCommandList.head-gCommandList.tail) & CMDBUF_SIZE_MASK);
}

idx_t command_buffer_put(CozmoCommand* data) {
  idx_t available = CMDBUF_CAPACITY-1 - command_buffer_count();
  if (available) {
    gCommandList.buffer[gCommandList.head++]=*data;
    gCommandList.head &= CMDBUF_SIZE_MASK;
    return available-1;
  }
  return 0;
}

idx_t command_buffer_get(CozmoCommand* data) {
  idx_t available = command_buffer_count();
  if (available) {
    *data = gCommandList.buffer[gCommandList.tail++];
    gCommandList.tail &= CMDBUF_SIZE_MASK;
    return available-1;
  }
  return 0;
}


/***** Command Tracker  **************************/
CozmoCommand gPending = {0};
CozmoCommand gActiveState = {0};
int gRemainingActiveCycles = 0;


void show_command(CozmoCommand* cmd) {
  ccc_debug_x("CCC {%d, [%d %d %d %d], %x}\n",
         cmd->repeat_count,
         cmd->motorValues[0],
         cmd->motorValues[1],
         cmd->motorValues[2],
         cmd->motorValues[3],
         cmd->printmask);
}

void PrepCommand(void) { memset(&gPending,0,sizeof(gPending)); }
void SetRepeats(int n) {gPending.repeat_count = n; }
void AddPrintFlag(int flag) { gPending.printmask |= flag; }
void AddMotorCommand(RobotMotor motor, uint8_t power_byte) {
  int8_t signed_power = (int8_t)*(&power_byte);
  float power = signed_power / 128.0;
  if (power > 1.0) { power = 1.0;}
  if (power < -1.0) { power = -1.0;}
  gPending.motorValues[motor] = 0x7FFF * power;
}
void SetHoldToken(char token)
{
    gPending.holdToken = token;
}
int ClearHoldToken(char token, int status) {
  if (token == gActiveState.holdToken) {
    gActiveState.holdToken=0;
    gActiveState.result = status;
    return 1;
  }
  ccc_debug("token mismatch %c != %c\n", token, gActiveState.holdToken);
  return 0;
}

void SubmitCommand() {
  ccc_debug_x("CCC submitting command");
  if (!gPending.repeat_count) { gPending.repeat_count=1;}
  show_command(&gPending);
  command_buffer_put(&gPending);
  PrepCommand(); //make sure it is cleared
}

int ActiveCycleCountdown(void) {
  return gActiveState.repeat_count;
}
int ActiveCyclesModify(int delta) {
  gActiveState.repeat_count+=delta;
  return gActiveState.repeat_count;
}

uint16_t ActivePrintmask(void) {
  return gActiveState.printmask;
}



/************* CIRCULAR BUFFER for outgoing contact text ***************/


#define TXTBUF_CAPACITY (1<<12)
#define TXTBUF_SIZE_MASK (TXTBUF_CAPACITY-1)


struct text_buffer_t {
//  struct ContactData buffer[TXTBUF_CAPACITY];
  unsigned char text[TXTBUF_CAPACITY];
  idx_t head;
  idx_t tail;
};

#pragma clang diagnostic ignored "-Wmissing-braces" //f'in Clang doesn't know how to init.
static struct text_buffer_t gOutgoingText = {0};

static inline idx_t contact_text_buffer_count() {
  return ((gOutgoingText.head-gOutgoingText.tail) & TXTBUF_SIZE_MASK);
}

idx_t contact_text_buffer_put(const unsigned char* text, idx_t len) {
  idx_t available = TXTBUF_CAPACITY-1 - contact_text_buffer_count();
  idx_t count = MIN(available, len);
  while (count-->0) {
    gOutgoingText.text[gOutgoingText.head++]=*text++;
    gOutgoingText.head &= TXTBUF_SIZE_MASK;
    len--;
  }
  return len;
}

idx_t contact_text_buffer_get(unsigned char data[], idx_t len) {
  idx_t available = contact_text_buffer_count();
  idx_t count = 0;
  len = MIN(available, len);
  while (count < len) {
    *data++ = gOutgoingText.text[gOutgoingText.tail++];
    gOutgoingText.tail &= TXTBUF_SIZE_MASK;
    count++;
  }
return count;
}

/***** logging interface  **************************/

int ReadLogN(uint8_t n) {
  if (n>=MAX_KNOWN_LOG) {
    return ERR_UNKNOWN;
  }
  char filename[MAX_FACTORY_FILENAME_LEN];
  sprintf(filename, FACTORY_FILENAME_TEMPLATE, n);
  gLogFd = open(filename, O_RDONLY);
  if (gLogFd <= 0) {
    ccc_debug("CCC log file open failed %d",errno);
    return ERR_SYSTEM; }
  print_response(":LOG %d\n", n);
  ccc_debug("CCC Setting Log Flag for (%d)",gLogFd);

  SetRepeats(1);
  AddPrintFlag(1<<show_LOGS);
  return ERR_PENDING;
}

int LogShowLine(void) {
  char logline[RESPONSE_CHUNK_SZ];
  int nchars = read(gLogFd, logline, RESPONSE_CHUNK_SZ-1);
  ccc_debug("CCC Showing log line of %d chars from (%d) ", nchars, gLogFd);
  if (nchars >0 ) {
    logline[nchars]='\0';
    print_response("%.*s", RESPONSE_CHUNK_SZ, logline);
  }
  else {
    ccc_debug("CCC closing logfile (%d)", gLogFd);
    close(gLogFd);
    gLogFd = 0;
  }
  return (nchars>0);
}


int emr_set(uint8_t index, uint32_t value);
int emr_get(uint8_t index, uint32_t* value);


/******* EMR interface  **************************/

int SetMedicalRecord(uint8_t index, uint32_t value)
{
  print_response("EMR %d := %08x\n", index, value);
  if (index == 0) { return ERR_SYSTEM; }
  int retval = emr_set(index, value);
  sync();
  return retval;
}

int GetMedicalRecord(uint8_t index)
{
  uint32_t value = 0;
  int err = emr_get(index, &value);
  print_response(":%08x @ EMR[%d]\n", value, index);
  return err;
}


//todo: collapse
int engine_test_run(uint8_t id, uint8_t args[4]);
int SendEngineCommand(uint8_t index, uint8_t v0, uint8_t v1, uint8_t v2, uint8_t v3){
  uint8_t args[4] = {v0, v1, v2, v3};
  int result = engine_test_run(index, args);
  if (result < 0) {
    SetHoldToken('e');
  }
  return result;
}

/***** version commands  **************************/


#define EMR_ESN_OFFSET 0  //first item
#define MAX_SERIAL_LEN 20
int get_esn(char buf[], int len)
{
  uint32_t value;
  int err = emr_get(EMR_ESN_OFFSET, &value);
  if (!err) {
    int nchars = snprintf(buf, len, "%08x", value);
    return (nchars>0 && nchars<len);
  }
  return 0;
}

static  struct VersionInfo bodyVersion_ = {0};
static bool gHasVersion = false;

void record_body_version( const struct VersionInfo* info)
{
  memcpy(&bodyVersion_, info, sizeof(bodyVersion_));
  ccc_debug_x("CCC clearing hold for body version");//
  gHasVersion = true;
  if (ClearHoldToken('b', ERR_OK)) {
    AddPrintFlag(1<<show_BSV);
  }
}

void get_body_version( struct VersionInfo* info_out)
{
  memcpy(info_out, &bodyVersion_, sizeof(bodyVersion_));
}


/********* command parsing **************/



int AddSensorCommand(uint8_t sensorId) { //return error
  if (sensorId >0 && sensorId < show_LAST_SENSOR) {
    AddPrintFlag(1<<sensorId);
    return ERR_OK;
  }
  return ERR_SYSTEM;
}


uint8_t handle_esn_command(const uint8_t args[])
{
  char esn_text[MAX_SERIAL_LEN];
  if ( get_esn(esn_text, sizeof(esn_text)) ) {
    print_response(":%s\n", esn_text);
    return ERR_OK;
  }
  return ERR_SYSTEM;
}


extern void get_body_version(struct VersionInfo*);
uint8_t handle_bsv_command(const uint8_t args[])
{
  if (!gHasVersion) {
    SetHoldToken('b');
    request_version();
  }
  else {
    AddPrintFlag(1<<show_BSV);
  }
  SetRepeats(BSV_LINES);

  return ERR_OK;
}


uint8_t handle_mot_command(const uint8_t args[])
{
  ccc_debug_x("CCC Handling mot command: %d %d ...\n", args[0], args[1]);
  SetRepeats(args[0]);
  AddSensorCommand(args[1]);
  AddMotorCommand(MOTOR_LEFT, args[2]);
  AddMotorCommand(MOTOR_RIGHT, args[3]);
  AddMotorCommand(MOTOR_LIFT, args[4]);
  AddMotorCommand(MOTOR_HEAD, args[5]);
  return ERR_OK;
}

uint8_t handle_get_command(const uint8_t args[])
{
  return handle_mot_command(args);
}


uint8_t handle_fcc_command(const uint8_t args[])
{
  //RunFccScript(args[0],args[1]);
  return ERR_PENDING;
}

uint8_t handle_rlg_command(const uint8_t args[])
{
  return ReadLogN(args[0]);
}

uint8_t handle_eng_command(const uint8_t args[])
{
  return SendEngineCommand(args[0], args[1], args[2], args[3], args[4]);
}

uint8_t handle_smr_command(const uint8_t args[]) {
  uint32_t value = args[4] << 24|
    (args[3] << 16) |
    (args[2] << 8)|
    (args[1] << 0);
  return SetMedicalRecord(args[0], value);
}
uint8_t handle_gmr_command(const uint8_t args[]) {
  return GetMedicalRecord(args[0]);
}


int SetLeds(const uint8_t* leds, int nleds);

uint8_t handle_led_command(const uint8_t args[])
{
  int i;
  uint8_t leds[12];
  //each nibble gets doubled.
  for (i=0;i<6;i++) {
    uint8_t nibble = (args[i]>>4)&0x0F;
    leds[2*i] = nibble<<4|nibble;
    nibble = (args[i])&0x0F;
    leds[2*i+1] = nibble<<4|nibble;
  }
  return SetLeds(leds, sizeof(leds));
}


#define SHUTDOWN_CYCLES 200   // 0.1 second
uint8_t handle_pwr_command(const uint8_t args[]) {

  switch (args[0]) {
    case 0:
      //no action
      break;
    case 1: //reboot
      start_overrride(400); //2 secs
      gShutdownCountdown = SHUTDOWN_CYCLES;

      break;
    case 2:  //shutdown
      hal_terminate();
      break;
    default:
      return ERR_SYNTAX;
  }
  return err_OK;
}

int gQuit = 0;
const char* handle_quit_command(const char* text, int len)
{
  gQuit = 1;
  return text;
}


#ifdef STANDALONE_UTILITY

void core_common_on_exit(void)
{
  hal_terminate();
  enable_kbhit(0);
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
}

#endif

#define REGISTER_COMMAND(s) {#s, handle_##s##_command}


typedef uint8_t (*CommandParser)(const uint8_t[]);

typedef struct CommandHandler_t {
  const char* name;
  const CommandParser handler;
} CommandHandler;


static const CommandHandler handlers[] = {
  REGISTER_COMMAND(esn),
  REGISTER_COMMAND(bsv),
  REGISTER_COMMAND(mot),
  REGISTER_COMMAND(get),
  REGISTER_COMMAND(fcc),
  REGISTER_COMMAND(rlg),
  REGISTER_COMMAND(eng),
//  REGISTER_COMMAND(lfe),
  REGISTER_COMMAND(smr),
  REGISTER_COMMAND(gmr),
  REGISTER_COMMAND(pwr),
  REGISTER_COMMAND(led),
 /* ^^ insert new commands here ^^ */
  {0}  /* MUST BE 0 TERMINATED */
};



uint8_t parse_command_text(char* cmd, int len) {
  if (len !=  FIXED_LINE_LEN) {
      return ERR_SYNTAX;
   }
   const CommandHandler* candidate = &handlers[0];
   while (candidate->name) {
      if (strncmp(cmd, candidate->name, FIXED_NAME_LEN)==0) {
        ccc_debug_x("matched %s\n", candidate->name);
        char* argstr = cmd+FIXED_NAME_LEN+1;
        uint8_t args[FIXED_ARG_COUNT];
        int i;
        for (i=0;i<FIXED_ARG_COUNT;i++)
        {
          char* end = argstr+FIXED_ARG_LEN;
          *end = '\0';
          long arg = strtol(argstr, &end, 16);
          if (end!=argstr+FIXED_ARG_LEN) { //did not convert expected chars
            return ERR_SYNTAX;
          }
          args[i]=arg;
          argstr = end+1;//next word, skips null.
        }
        uint8_t result = candidate->handler(args);
        return result;
      }
      candidate++;
   }
   return ERR_UNKNOWN;
}


#define LINEBUFSZ 512

char* handle_overflow(char* linebuf, int maxlen) {
  ccc_debug("TOO MANY CHARACTERS, truncating to %d\n", maxlen);
  char* endl = linebuf-maxlen-1;
  *endl = '\n';
  return endl;
}

#ifdef STANDALONE_TEST
int kbd_command_process(void) {
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  if (kbhit()) {
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    if (nread<0) { return 1; }
    ccc_debug("%.*s", nread, linebuf+linelen);
    fflush(0);


/* ** */
    while (nread) {
      char* endl = memchr(linebuf+linelen, '\n', nread);
      linelen+=nread;
      nread = 0; //assume no more chars this loop
      if (linelen >= LINEBUFSZ)
      {
        endl = handle_overflow(linebuf,LINEBUFSZ);
      }
      if (endl) {
        parse_command_text(linebuf, endl-linebuf);
        endl++;
        if (endl-linebuf < linelen) { //more chars remain
          nread = linelen - (endl-linebuf); //act like they are new
          memmove(linebuf, endl, nread);    //by moving to beginning of buffer.
        }
        linelen = 0;
      }
    }
  }


  return 0;
}
#endif

int printable_string(char outbuf[], int outlen, const uint8_t* input, int inlen) {
  int i;
  for (i=0;i<inlen&&i<outlen;i++){
    char c = input[i];
    outbuf[i] = isprint(c)?c:'*';
  }
  assert(i<=outlen);
  if (i==outlen) { i--; }
  outbuf[i++]='\0';
  return i;
}

bool gather_contact_text(const char* contactData, int len)
{
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  bool cmd_detected = false;
  while (*contactData && len-->0) {
    unsigned char c = *contactData++;

//    print_response("%c",c);
    if (c=='\n' || c=='\r') {
      linebuf[linelen]='\0';
      ccc_debug("CCC Line recieved [ %.*s ]", linelen, linebuf);
      if (linelen >= 5 && linebuf[0]=='>' && linebuf[1]=='>') {
        ccc_debug_x("good prefix\n");
        int status = parse_command_text(linebuf+2, linelen-2);
        ccc_debug_x("replying <<%.3s %d\n",linebuf+2, status);
        snprintf(gPending.cmd, sizeof(gPending.cmd), "%.3s", linebuf+2);
        gPending.result = status;
        cmd_detected = true ;
        SubmitCommand( );
      }
      else {
        ccc_debug_x("non-command line\n");
      }
      linelen = 0;
    }
    else if (linelen < LINEBUFSZ) {
      if (isprint(c) ) {
        linebuf[linelen++]=c;
      }
    }
    else {
      ccc_debug("contact buffer overflow");
      linelen = 0;
    }
  }
  return cmd_detected;
}


const void* get_a_frame(int32_t timeout_ms)
{
  timeout_ms *= 1000.0/HAL_SERIAL_POLL_INTERVAL_US;
  const struct SpineMessageHeader* hdr;
  do {
    hdr = hal_read_frame();
    if (timeout_ms>0 && --timeout_ms==0) {
      return NULL;
    }
  }
  while (!hdr);
  return hdr;
}




int SetLeds(const uint8_t* leds, int nleds) {
  assert(nleds <= sizeof(gHeadData.lightState.ledColors));
  memcpy(&gHeadData.lightState.ledColors, leds, nleds);
  start_overrride(400); //2 secs
  return 0;
}


void populate_outgoing_frame(void) {
  gHeadData.framecounter++;
  if (ActiveCycleCountdown()) {
    int i;
    for (i=0;i<4;i++) {
      gHeadData.motorPower[i] = gActiveState.motorValues[i];
    }
  }
  else {
    memset(gActiveState.motorValues, 0, sizeof(gActiveState.motorValues));
    memset(gHeadData.motorPower, 0, sizeof(gHeadData.motorPower));
  }
}

#ifdef STANDALONE_TEST
#define print_response printf
#else

int print_response(const char* format, ...) {
  struct ContactData response = {{0}};
  va_list argptr;
  va_start(argptr, format);

  if (gRunState != runstate_NORMAL) {
    int retval = vprintf(format, argptr);
    va_end(argptr);
    return retval;
  }


  int nchars = vsnprintf((char*)response.data,
                         sizeof(response.data),
                         format, argptr);

 va_end(argptr);
 if (nchars >= 0)  { //successful write
   int remainder = sizeof(response.data) - nchars;
   int actual_len =  (remainder >= 0)  ? nchars : sizeof(response.data);
   ccc_debug_x("CCC preparing response [ %s ]", response.data);
   contact_text_buffer_put(response.data, actual_len);
   if (remainder < 0) { // the string was truncated
     return nchars + print_response("...");
   }
 }
 return nchars;
}


#endif

void show_body_version(int linect) {
  switch (linect) {
    case 0: {
      print_response(":%08x %08x\n", bodyVersion_.hw_revision, bodyVersion_.hw_model);
      break;
    }
    case 1:
    case 2: {
      uint32_t lo_part = *((uint32_t*)&bodyVersion_.ein[0+8*(linect-1)]);
      uint32_t hi_part = *((uint32_t*)&bodyVersion_.ein[4+8*(linect-1)]);
      print_response(":%08x %08x\n", lo_part, hi_part);
      break;
    }
    case 3:
    case 4: {
      uint32_t lo_part = *((uint32_t*)&bodyVersion_.app_version[0+8*(linect-3)]);
      uint32_t hi_part = *((uint32_t*)&bodyVersion_.app_version[4+8*(linect-3)]);
      print_response(":%08x %08x\n", lo_part, hi_part);
      break;
    }
    case 5:{
      char sanitized_str[16+1];
      printable_string(sanitized_str, 16,
                       bodyVersion_.app_version,
                       sizeof(bodyVersion_.app_version));
      print_response("%.16s\n", sanitized_str);
      break;
    }
    default:
      assert(linect < BSV_LINES);
  }
}




uint16_t show_legend(uint16_t mask) {
  if (mask & (1<<show_MOTOR0)) {
    print_response("LEFT pos speed\n");
  }
  if (mask & (1<<show_MOTOR1)) {
    print_response("RIGHT pos speed\n");
  }
  if (mask & (1<<show_MOTOR2)) {
    print_response("LIFT pos speed\n");
  }
  if (mask & (1<<show_MOTOR3)) {
    print_response("HEAD pos speed\n");
  }
  if (mask & (1<<show_CLIFF)) {
    print_response("CLIFF fl fr br bl \n");
  }
  if (mask & (1<<show_BAT)) {
    print_response("BAT volt temp \n");
  }
  if (mask & (1<<show_PROX)) {
    print_response("rangeMM spadCt sigRt ambRt\n");
  }
  if (mask & (1<<show_TOUCH)) {
    print_response("TOUCH BTN\n");
  }
  if (mask & (1<<show_RSSI)) {
    print_response("RSSI\n");
  }
  if (mask & (1<<show_PACKETS)) {
    print_response("PACKETCNT\n");
  }
//  print_response("\n");
  return mask;
}


static const int SYSCON_CLOCK_FREQ = 48000000;
//static const float HAL_SEC_PER_TICK = (1.0 / 256) / SYSCON_CLOCK_FREQ;
static const int HAL_TICKS_PER_US =  (SYSCON_CLOCK_FREQ / 1000000) * 256;


void show_motor(int motor_id, const struct MotorState motor[]) {
  assert(motor_id >= 0 && motor_id < 4);
  static int32_t lastpos[4] = {0};
  const struct MotorState* m = &motor[motor_id];
  int32_t dist = m->delta * 1000000;
  int32_t microsec = m->time / HAL_TICKS_PER_US;
  int32_t speed = (microsec!=0 && m->position != lastpos[motor_id]) ?
    (dist/microsec) : 0;
  ccc_debug_x("CCC motor pos/speed:  %d/ %d in %d -> %d", m->position, m->delta, m->time, speed);
  print_response(":%d %d %d\n",
                 m->position,
                 speed, m->delta);
  lastpos[motor_id] = m->position;

}


void show_packet_count(void) {
  print_response(":%d\n", -1);

}
void show_rssi(void) {
  print_response(":%d\n", -128);

}

void show_debug_counter(void) {
  static int debug_count = 1000;
  print_response(":%04d %04d %04d %04d\n",
                 debug_count, debug_count+1, debug_count+2, debug_count+3);
  debug_count+=4;
  debug_count%=10000;
}


void process_incoming_frame(struct BodyToHead* bodyData)
{
  static uint16_t oldmask = 0;
  if (ActiveCycleCountdown()) {
    uint16_t printmask = ActivePrintmask();
    ccc_debug_x("CCC printmask is active: %x\n",printmask);
    if (printmask != oldmask) {
      oldmask = show_legend(printmask);
    }

    if (printmask & (1<<show_MOTOR0)) {
      show_motor(0, bodyData->motor);
    }
    if (printmask & (1<<show_MOTOR1)) {
      show_motor(1, bodyData->motor);
    }
    if (printmask & (1<<show_MOTOR2)) {
      show_motor(2, bodyData->motor);
    }
    if (printmask & (1<<show_MOTOR3)) {
      show_motor(3, bodyData->motor);
    }

    if (printmask & (1<<show_CLIFF)) {
      print_response(":%u %u %u %u\n",
             bodyData->cliffSense[0],
             bodyData->cliffSense[1],
             bodyData->cliffSense[2],
             bodyData->cliffSense[3] );
    }
    if (printmask & (1<<show_BAT)) {
      print_response(":%i %i\n",
                     bodyData->battery.main_voltage,
                     bodyData->battery.temperature);
    }
    if (printmask & (1<<show_PROX)) {
      print_response(":%u %u %u %u\n",
                     bodyData->proximity.rangeMM,
                     bodyData->proximity.spadCount,
                     bodyData->proximity.signalRate,
                     bodyData->proximity.ambientRate);

    }
    if (printmask & (1<<show_TOUCH)) {
      print_response(":%u %u\n",
             bodyData->touchLevel[0],
             bodyData->touchLevel[1]);
    }

    if (printmask & (1<<show_BSV)) {
      show_body_version(BSV_LINES - ActiveCycleCountdown()); //convert to countup
    }
    if (printmask & (1<<show_RSSI)) {
      show_rssi();
    }
    if (printmask & (1<<show_PACKETS)) {
      show_packet_count();
    }
    if (printmask & (1<<show_DEBUG_COUNTER)) {
      show_debug_counter();
    }
    if (printmask & (1<<show_LOGS)) {
      if (LogShowLine()) {
        gActiveState.result = ERR_OK;
        ActiveCyclesModify(1); //add 1 to keep running
      }
    }

    if (!gActiveState.holdToken && ActiveCyclesModify(-1) == 0) {
      //clear print request at end
//      ActiveCycleEnd();
      ccc_debug("CCC command done %d", gActiveState.result);
      print_response("<<%.3s %d\n", gActiveState.cmd, gActiveState.result);
      gActiveState.printmask = 0;
      oldmask = 0; //force headers every new print.
    }
  }
  if (gShutdownCountdown) {
    if (--gShutdownCountdown==0) {
      hal_terminate();
    }
  }
}

#define CCC_COOLDOWN_TIME 100 //half second

void start_overrride(int count) {
  if (count < gRemainingActiveCycles) count = gRemainingActiveCycles;
  ccc_debug_x("CCC active for %d cycles", count + ActiveCycleCountdown());
  gRemainingActiveCycles = count;
}

void stop_override(void) {
  ccc_debug_x("CCC deactivating");
  gRemainingActiveCycles = 0;

}

int run_commands(void) {
  if (ActiveCycleCountdown() == 0) {
    //when no active command, start countingdown active time.
    if (gRemainingActiveCycles>0) {
      --gRemainingActiveCycles;
      if (gRemainingActiveCycles==0) { stop_override(); }
    }
    //when no active command, check if there is another one pending
    if (command_buffer_count() > 0) {
      command_buffer_get(&gActiveState);
      //reset active time
      ccc_debug_x("CCC executing command");
      start_overrride(CCC_COOLDOWN_TIME);
      show_command(&gActiveState);
    }
    else {
    }
  }
  return (ActiveCycleCountdown() == 0 && gQuit);
}


#ifdef STANDALONE_UTILITY

int main(int argc, const char* argv[])
{
  bool exit = false;

  signal(SIGINT, safe_quit);
  signal(SIGKILL, safe_quit);

  lcd_init();
  lcd_set_brightness(20);
  display_init();

  int errCode = hal_init(SPINE_TTY, SPINE_BAUD);
  if (errCode) { error_exit(errCode, "hal_init"); }

  enable_kbhit(1);

  hal_set_mode(RobotMode_RUN);

  //kick off the body frames
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));

  usleep(5000);
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));


  while (!exit)
  {
    start_overrride(1000);; //Force on
    kbd_command_process();

    exit = run_commands();

    const struct SpineMessageHeader* hdr = get_a_frame(10);
    if (!hdr) {
      struct BodyToHead fakeData = {0};
      process_incoming_frame(&fakeData);
    }
    else {
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         struct BodyToHead* bodyData = (struct BodyToHead*)(hdr+1);
         populate_outgoing_frame();
         process_incoming_frame(bodyData);
         hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));
      }
      else if (hdr->payload_type == PAYLOAD_CONT_DATA) {
        struct ContactData* contactData = (struct ContactData*)(hdr+1);
        gather_contact_text((char*)contactData->data, sizeof(contactData->data));
      }
      else {
        ccc_debug("got unexpected header %x\n", hdr->payload_type);
      }
    }

  }

  core_common_on_exit();

  return 0;
}

#else

ShutdownFunction gShutdownFp = NULL;

bool ccc_commander_is_active(void) {
  if (gRunState == runstate_CMDLINE_PENDING) {
    gRunState = runstate_CMDLINE_ACTIVE;
    gather_contact_text("\n",1); //newline to force execution
  }

  return gRemainingActiveCycles > 0 || gActiveState.repeat_count > 0;
}

void ccc_data_process(const struct ContactData* data)
{
  ccc_debug_x("CCC data frame rcvd\n");
  if (gather_contact_text((const char*)data->data,
                          sizeof(data->data))) {
    start_overrride( CCC_COOLDOWN_TIME);
  }
}

void ccc_payload_process(struct BodyToHead* data)
{
  static uint32_t framecounter = 0;
  if (data->framecounter != framecounter) {
    framecounter = data->framecounter;
    process_incoming_frame(data);
    run_commands();
  }
}


struct HeadToBody* ccc_data_get_response(void) {
  populate_outgoing_frame();
  return &gHeadData;
}

#define CD_DATA_SLACK 4  //use 28 out of 32 chars to prevent syscon buffer overflow
static struct ContactData response;
struct ContactData* ccc_text_response(void) {

  memset(response.data, SLUG_PAD_CHAR, SLUG_PAD_SIZE);
  const int space_remaining = sizeof(response.data)-SLUG_PAD_SIZE;
  uint8_t* text_start = response.data+SLUG_PAD_SIZE;

  idx_t nchars = contact_text_buffer_get(text_start, space_remaining-CD_DATA_SLACK);
  if (nchars) {
    memset(text_start+nchars,'\0', space_remaining-nchars); //pad remainder
    ccc_debug("CCC transmitting response [ %s ]", response.data);
    return &response;
  }
  return NULL;
}


void ccc_parse_command_line(int argc, const char* argv[])
{
  ccc_debug_x("CCC Parsing %d cmdline args\n", argc);
  start_overrride( CCC_COOLDOWN_TIME );
  gRunState = runstate_CMDLINE_PENDING;
  gather_contact_text(">>",2);
  while (argc-->0) {
    gather_contact_text(*argv, strlen(*argv));
    gather_contact_text(" ",1);
    argv++;
  }
}

void ccc_set_shutdown_function(ShutdownFunction fp) {
  gShutdownFp = fp;
  //TODO: actually use this to shut down at end
}


void ccc_test_result(int status, const char string[32])
{
  if (ClearHoldToken('e', status)) {
    print_response("%.32s", string);
  }

}

#endif
