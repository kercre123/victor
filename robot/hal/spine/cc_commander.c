#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>



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


const struct SpineMessageHeader* hal_read_frame();
void start_overrride(int count);

enum {
  show_ENCODER,
  show_SPEED,
  show_CLIFF,
  show_BAT,
  show_PROX,
  show_TOUCH,
  SHOW_LAST
};

struct HeadToBody gHeadData = {0};

typedef struct CozmoCommand_t {
  uint16_t repeat_count;
  int16_t motorValues[MOTOR_COUNT];
  uint16_t printmask;
  char reply[30];
} CozmoCommand;

CozmoCommand gActiveState = {0};
int gRemainingActiveCycles = 0;

//static struct ContactData gResponse;
//int gRespPending = 0;


#define MAX_SERIAL_LEN 20
int get_esn(char buf[], int len)
{
  int fd = open("/sys/devices/virtual/android_usb/android0/iSerial", O_RDONLY);
  if (len > MAX_SERIAL_LEN) len = MAX_SERIAL_LEN;
  int nchars = read(fd, buf, len-1);
  close(fd);
  if (nchars > 0) {
    buf[nchars]='\0';
  }
  return nchars;
  return 0;
}

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

/************* CIRCULAR BUFFER for outgoing contact text ***************/

#define MIN(a,b) ((a)<(b)?(a):(b))

#define TXTBUF_CAPACITY (1<<8)
#define TXTBUF_SIZE_MASK (TXTBUF_CAPACITY-1)


struct text_buffer_t {
  struct ContactData buffer[TXTBUF_CAPACITY];
  idx_t head;
  idx_t tail;
};

#pragma clang diagnostic ignored "-Wmissing-braces" //F'in Clang doesn't know how to init.
static struct text_buffer_t gOutgoingText = {0};

static inline idx_t contact_text_buffer_count() {
  return ((gOutgoingText.head-gOutgoingText.tail) & TXTBUF_SIZE_MASK);
}

idx_t contact_text_buffer_put(struct ContactData* data) {
  idx_t available = TXTBUF_CAPACITY-1 - contact_text_buffer_count();
  if (available) {
    gOutgoingText.buffer[gOutgoingText.head++]=*data;
    gOutgoingText.head &= TXTBUF_SIZE_MASK;
    return available-1;
  }
  return 0;
}

idx_t contact_text_buffer_get(struct ContactData* data) {
  idx_t available = contact_text_buffer_count();
  if (available) {
    *data = gOutgoingText.buffer[gOutgoingText.tail++];
    gOutgoingText.tail &= TXTBUF_SIZE_MASK;
    return available;
  }
  return 0;
}


/***** pending commands  **************************/
CozmoCommand gPending = {0};

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
void AddMotorCommand(RobotMotor motor, float power) {
  if (power > 1.0) { power = 1.0;}
  if (power < -1.0) { power = -1.0;}
  gPending.motorValues[motor] = 0x7FFF * power;
}
void SubmitCommand(bool cmd_rcvd, const char* text, int len) {
  ccc_debug_x("CCC submitting command");
  if (!gPending.repeat_count) { gPending.repeat_count=1;}
  show_command(&gPending);
  command_buffer_put(&gPending);
  PrepCommand(); //make sure it is cleared
}

/********* command parsing **************/

const char* skip_ws(const char* text, int len)
{
  const char* next = text;
  while ( (next < text+len) && isspace(*next)) { next++;}
  return next;
}

const char* handle_repeat_command(const char* text, int len)
{
  char* end;
  long nrepeats = strtol(text, &end, 10);
  if (end!=text && end<=text+len) //converted some chars
  {
    SetRepeats(nrepeats);
  }
  return end;
}

const char* handle_motor(const char* text, int len, RobotMotor motor)
{
  char* end;
  double power = strtof(text, &end);
  if (end!=text && end <= text+len) //converted some chars
  {
    AddMotorCommand(motor, power);
  }
  return end;
}

const char* handle_head_command(const char* text, int len)
{
  return handle_motor(text, len, MOTOR_HEAD);
}
const char* handle_lift_command(const char* text, int len)
{
  return handle_motor(text, len, MOTOR_LIFT);
}
const char* handle_left_command(const char* text, int len)
{
  return handle_motor(text, len, MOTOR_LEFT);
}
const char* handle_right_command(const char* text, int len)
{
  return handle_motor(text, len, MOTOR_RIGHT);
}

const char* handle_enc_command(const char* text, int len)
{
  AddPrintFlag(1<<show_ENCODER);
  return text;
}

const char* handle_prox_command(const char* text, int len)
{
  AddPrintFlag(1<<show_PROX);
  return text;
}

const char* handle_touch_command(const char* text, int len)
{
  AddPrintFlag(1<<show_TOUCH);
  return text;
}

const char* handle_speed_command(const char* text, int len)
{
  AddPrintFlag(1<<show_SPEED);
  return text;
}

const char* handle_bat_command(const char* text, int len)
{
  AddPrintFlag(1<<show_BAT);
  return text;
}

const char* handle_cliff_command(const char* text, int len)
{
  AddPrintFlag(1<<show_CLIFF);
  return text;
}

const char* handle_getserial_command(const char* text, int len)
{
  char esn_text[MAX_SERIAL_LEN];
  if ( get_esn(esn_text, sizeof(esn_text)) ) {
    print_response("ESN = %s\n", esn_text);
  }
  else {
    print_response("ESN : READ_ERR\n");
  }

  return text;
}

const char* handle_mode_command(const char* text, int len)
{
  hal_set_mode(RobotMode_RUN);
  return text;
}

const char* handle_override_command(const char* text, int len)
{
  char* end;
  long enable_count = strtol(text, &end, 10);
  if (end!=text && end<=text+len) //converted some chars
  {
    start_overrride(enable_count);
  }
  return end;
}


int gQuit = 0;
const char* handle_quit_command(const char* text, int len)
{
  gQuit = 1;
  return text;
}


#ifdef STANDALONE_UTILITY

void on_exit(void)
{
  hal_terminate();
  enable_kbhit(0);
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
}

#endif

#define REGISTER_COMMAND(s) {#s, sizeof(#s)-1, handle_##s##_command}


typedef const char* (*CommandParser)(const char*, int);

typedef struct CommandHandler_t {
  const char* name;
  const int len;
  const CommandParser handler;
} CommandHandler;


static const CommandHandler handlers[] = {
  REGISTER_COMMAND(repeat),
  {"r",1,handle_repeat_command}, //shortcut for repeat
  REGISTER_COMMAND(head),
  REGISTER_COMMAND(lift),
  REGISTER_COMMAND(left),
  REGISTER_COMMAND(right),
  REGISTER_COMMAND(enc),
  REGISTER_COMMAND(bat),
  REGISTER_COMMAND(speed),
  REGISTER_COMMAND(cliff),
  REGISTER_COMMAND(prox),
  REGISTER_COMMAND(touch),
  REGISTER_COMMAND(getserial),
  {"esn",3,handle_getserial_command}, //shortcut for repeat
  REGISTER_COMMAND(mode),
  REGISTER_COMMAND(override),
  REGISTER_COMMAND(quit),
 /* ^^ insert new commands here ^^ */
  {0}
};


//returns true if at least one valid command
bool parse_command_text(const char* command, int len)
{
  bool cmd_rcvd = false;
//  const char *orig_cmd = command;
  PrepCommand();
  while (len > 0)  {
    const char* next_word = NULL;
    const CommandHandler* candidate = &handlers[0];

    ccc_debug("CCC HANDLING [ %.*s ] ...", len, command);

    while (candidate->name) {
      if (len >= candidate->len &&
          strncmp(command, candidate->name, candidate->len)==0)
      {
        ccc_debug("CCC MATCH %s [ %.*s ] \n", candidate->name, len-candidate->len, command+candidate->len);
        if (!cmd_rcvd) {
          snprintf(gPending.reply, sizeof(gPending.reply), "<<%.*s 0 ",
                   candidate->len, candidate->name);
          cmd_rcvd = true;
        }
        next_word =  candidate->handler(command+candidate->len, len-candidate->len);
        break;
      }
      candidate++;
    }
    //not recognized, echo back invalid command with error code
    if(!candidate->name) {
      ccc_debug("UNKNOWN command: %.*s\n", len, command);
      next_word = memchr(command, ' ', len);
      int n = (next_word)?  next_word-command : len;
      print_response("<<%.*s -1\n", n, command);
    }
    if (next_word) {
      len -= next_word-command;
      command = skip_ws(next_word, len);
      len -= command-next_word;
    }
    else {
      len = 0;
    }
  }
  if (cmd_rcvd) {
    SubmitCommand(cmd_rcvd, command,len);
  }
  return cmd_rcvd;
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

bool gather_contact_text(const char* contactData, int len)
{
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  bool cmd_rcvd = false;
  while (*contactData && len-->0) {
    char c = *contactData++;


    printf(".%c",c);
    if (c=='\n' || c=='\r') {
      linebuf[linelen]='\0';
      ccc_debug("CCC Line recieved [ %.*s ]", linelen, linebuf);
      if (linebuf[0]=='>' && linebuf[1]=='>') {
        cmd_rcvd = cmd_rcvd || parse_command_text(linebuf+2, linelen-2);
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
  return cmd_rcvd;
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


void populate_outgoing_frame(void) {
  gHeadData.framecounter++;
  if (gActiveState.repeat_count) {
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

#define SLUG_PAD_CHAR 0xFF
#define SLUG_PAD_SIZE 4
int print_response(const char* format, ...) {
  struct ContactData response = {{0}};
  va_list argptr;
  va_start(argptr, format);

  memset(response.data, SLUG_PAD_CHAR, SLUG_PAD_SIZE);

 int nchars = vsnprintf((char*)response.data+SLUG_PAD_SIZE,
                        sizeof(response.data)-SLUG_PAD_SIZE,
                        format, argptr);

 va_end(argptr);
 if (nchars >= 0)  {
   nchars+=SLUG_PAD_SIZE; //this is the end of valid data in the packet
   memset(response.data+nchars, 0, sizeof(response.data)-nchars);

   ccc_debug_x("CCC preparing response [ %s ]", response.data);
   contact_text_buffer_put(&response);
 }
  return nchars;
}


#endif


uint16_t show_legend(uint16_t mask) {
  print_response("frame ");
  if (mask & (1<<show_ENCODER)) {
    print_response("left_enc right_enc lift_enc head_enc ");
  }
  if (mask & (1<<show_SPEED)) {
    print_response("left_spd right_spd lift_spd head_spd ");
  }
  if (mask & (1<<show_CLIFF)) {
    print_response("fl_cliff fr_cliff br_cliff bl_cliff ");
  }
  if (mask & (1<<show_BAT)) {
    print_response("bat ");
  }
  if (mask & (1<<show_PROX)) {
    print_response("rangeMM ");
  }
  if (mask & (1<<show_TOUCH)) {
    print_response("touch0 touch1 ");
  }
  print_response("\n");
  return mask;
}


void process_incoming_frame(struct BodyToHead* bodyData)
{
  static uint16_t oldmask = 0;
  if (gActiveState.repeat_count) {
    if (gActiveState.printmask != oldmask) {
      oldmask = show_legend(gActiveState.printmask);
    }

    if (gActiveState.repeat_count == 1) {
      print_response("%s", gActiveState.reply);
    }
    else {
      print_response("%d ", bodyData->framecounter);
    }

    if (gActiveState.printmask & (1<<show_ENCODER)) {
      print_response("%d %d %d %d ",
             bodyData->motor[0].position,
             bodyData->motor[1].position,
             bodyData->motor[2].position,
             bodyData->motor[3].position );
    }
    if (gActiveState.printmask & (1<<show_SPEED)) {
      float speeds[4];
      int i;
      for (i=0;i<4;i++) {
        speeds[i]= bodyData->motor[i].time ?
          ((float)bodyData->motor[i].delta / bodyData->motor[i].time) : 0;
      }
      print_response("%f %f %f %f ",
             speeds[0],
             speeds[1],
             speeds[2],
             speeds[3] );
    }
    if (gActiveState.printmask & (1<<show_CLIFF)) {
      print_response("%d %d %d %d ",
             bodyData->cliffSense[0],
             bodyData->cliffSense[1],
             bodyData->cliffSense[2],
             bodyData->cliffSense[3] );
    }
    if (gActiveState.printmask & (1<<show_BAT)) {
      print_response("%d ",
             bodyData->battery.battery);
    }
    if (gActiveState.printmask & (1<<show_PROX)) {
      print_response("%d",
             bodyData->proximity.rangeMM);
    }
    if (gActiveState.printmask & (1<<show_TOUCH)) {
      print_response("%d %d",
             bodyData->touchLevel[0],
             bodyData->touchLevel[1]);
    }
    print_response("\n");

    if (--gActiveState.repeat_count == 0) {
      //clear print request at end
      gActiveState.printmask = 0;
    }

  }
}

#define CCC_COOLDOWN_TIME 100 //half second

void start_overrride(int count) {
  ccc_debug_x("CCC active for %d cycles", count + gActiveState.repeat_count);
  gRemainingActiveCycles = count;
}

void stop_override(void) {
  ccc_debug_x("CCC deactivating");
  gRemainingActiveCycles = 0;

}

int run_commands(void) {
  if (gActiveState.repeat_count == 0) {
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
  return (gActiveState.repeat_count == 0 && gQuit);
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

  on_exit();

  return 0;
}

#else


bool ccc_commander_is_active(void) {
  return gRemainingActiveCycles > 0 || gActiveState.repeat_count > 0;
}

void ccc_data_process(const struct ContactData* data)
{
  ccc_debug_x("CCC data frame rcvd\n");
  start_overrride( CCC_COOLDOWN_TIME);
  if (gather_contact_text((const char*)data->data,
                          sizeof(data->data))) {
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


static struct ContactData response;
struct ContactData* ccc_text_response(void) {
  if (contact_text_buffer_get(&response)) {
    ccc_debug("CCC transmitting response [ %s ]", response.data);
    return &response;
  }
  /* if  (ccc_commander_is_active() && gRespPending) { */
  /*   ccc_debug_x("CCC transmitting response [ %s ]", gResponse.data); */
  /*   gRespPending = 0; */
  /*   return &gResponse; */
  /* } */
  return NULL;
}

#endif
