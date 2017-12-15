#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

#include "core/common.h"
#include "core/lcd.h"
#include "helpware/kbhit.h"
#include "helpware/display.h"
#include "spine/spine_hal.h"
#include "schema/messages.h"


const struct SpineMessageHeader* hal_read_frame();

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
  int repeat_count;
  int16_t motorValues[MOTOR_COUNT];
  uint16_t printmask;
} CozmoCommand;

CozmoCommand gActiveState = {0};



/************* CIRCULAR BUFFER for commands ***************/

#define MIN(a,b) ((a)<(b)?(a):(b))

typedef uint_fast16_t idx_t;
#define CB_CAPACITY (1<<8)
#define CB_SIZE_MASK (CB_CAPACITY-1)

static struct circular_buffer_t {
  CozmoCommand buffer[CB_CAPACITY];
  idx_t head;
  idx_t tail;
} gCommandList = {{{0}},0,0};

static inline idx_t circular_buffer_count() {
  return ((gCommandList.head-gCommandList.tail) & CB_SIZE_MASK);
}

idx_t circular_buffer_put(CozmoCommand* data) {
  idx_t available = CB_CAPACITY-1 - circular_buffer_count();
  if (available) {
    gCommandList.buffer[gCommandList.head++]=*data;
    gCommandList.head &= CB_SIZE_MASK;
    return available-1;
  }
  return 0;
}

idx_t circular_buffer_get(CozmoCommand* data) {
  idx_t available = circular_buffer_count();
  if (available) {
    *data = gCommandList.buffer[gCommandList.tail++];
    gCommandList.tail &= CB_SIZE_MASK;
    return available-1;
  }
  return 0;
}


/***** pending commands  **************************/
CozmoCommand gPending = {0};

void show_command(CozmoCommand* cmd) {
  printf("{%d, [%d %d %d %d], %x}\n",
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
void SubmitCommand(void) {
  printf("submitting command");
  show_command(&gPending);
  if (!gPending.repeat_count) { gPending.repeat_count=1;}
  circular_buffer_put(&gPending);
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

const char* handle_mode_command(const char* text, int len)
{
  hal_set_mode(RobotMode_RUN);
  return text;
}


int gQuit = 0;
const char* handle_quit_command(const char* text, int len)
{
  gQuit = 1;
  return text;
}


void on_exit(void)
{
  hal_terminate();
  enable_kbhit(0);
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
}


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
  REGISTER_COMMAND(mode),
  REGISTER_COMMAND(quit),
 /* ^^ insert new commands here ^^ */
  {0}
};


void parse_command_text(const char* command, int len)
{
  PrepCommand();
  while (len > 0)  {
    const char* next_word = NULL;
    const CommandHandler* candidate = &handlers[0];

    printf("HANDLING [ %.*s ] ...", len, command);

    while (candidate->name) {
      if (len >= candidate->len &&
          strncmp(command, candidate->name, candidate->len)==0)
      {
//        printf("MATCH %s [ %.*s ] \n", candidate->name, len-candidate->len, command+candidate->len);
        next_word =  candidate->handler(command+candidate->len, len-candidate->len);
        break;
      }
      candidate++;
    }
    //not recognized, echo back invalid command with error code
    if(!candidate->name) {
      printf("UNKNOWN command: %.*s\n", len, command);
      next_word = memchr(command, ' ', len);
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
  SubmitCommand();
}

#define LINEBUFSZ 512

char* handle_overflow(char* linebuf, int maxlen) {
  printf("TOO MANY CHARACTERS, truncating to %d\n", maxlen);
  char* endl = linebuf-maxlen-1;
  *endl = '\n';
  return endl;
}

int kbd_command_process(void) {
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  if (kbhit()) {
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    if (nread<0) { return 1; }
    printf("%.*s", nread, linebuf+linelen);
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


int gather_contact_text(const char* contactData, int len)
{
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  while (*contactData && len-->0) {
    char c = *contactData++;
    printf(".%c",c);
    if (c=='\n' || c=='\r') {
      linebuf[linelen]='\0';
      parse_command_text(linebuf, linelen);
      linelen = 0;
    }
    else if (linelen < LINEBUFSZ) {
      linebuf[linelen++]=c;
    }
    else {
      printf("contact buffer overflow");
      linelen = 0;
    }
  }
  return 0;
}


const void* get_a_frame(int32_t timeout_ms)
{
  timeout_ms *= 1000.0/SERIAL_POLL_INTERVAL_US;
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

uint16_t show_legend(uint16_t mask) {
  printf("frame ");
  if (mask & (1<<show_ENCODER)) {
    printf("left_enc right_enc lift_enc head_enc ");
  }
  if (mask & (1<<show_SPEED)) {
    printf("left_spd right_spd lift_spd head_spd ");
  }
  if (mask & (1<<show_CLIFF)) {
    printf("fl_cliff fr_cliff br_cliff bl_cliff ");
  }
  if (mask & (1<<show_BAT)) {
    printf("bat ");
  }
  if (mask & (1<<show_PROX)) {
    printf("rangeMM ");
  }
  if (mask & (1<<show_TOUCH)) {
    printf("touch0 touch1 ");
  }
  printf("\n");
  return mask;
}


void process_incoming_frame(struct BodyToHead* bodyData)
{
  static uint16_t oldmask = 0;
  if (gActiveState.repeat_count) {
    if (gActiveState.printmask != oldmask) {
      oldmask = show_legend(gActiveState.printmask);
    }

    printf("%d ", bodyData->framecounter);

    if (gActiveState.printmask & (1<<show_ENCODER)) {
      printf("%d %d %d %d ",
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
      printf("%f %f %f %f ",
             speeds[0],
             speeds[1],
             speeds[2],
             speeds[3] );
    }
    if (gActiveState.printmask & (1<<show_CLIFF)) {
      printf("%d %d %d %d ",
             bodyData->cliffSense[0],
             bodyData->cliffSense[1],
             bodyData->cliffSense[2],
             bodyData->cliffSense[3] );
    }
    if (gActiveState.printmask & (1<<show_BAT)) {
      printf("%d ",
             bodyData->battery.battery);
    }
    if (gActiveState.printmask & (1<<show_PROX)) {
      printf("%d",
             bodyData->proximity.rangeMM);
    }
    if (gActiveState.printmask & (1<<show_TOUCH)) {
      printf("%d %d",
             bodyData->touchLevel[0],
             bodyData->touchLevel[1]);
    }
    #endif
    printf("\n");
    if (--gActiveState.repeat_count == 0) {
      //clear print request at end
      gActiveState.printmask = 0;
    }
  }
}

int run_commands(void) {
  if (gActiveState.repeat_count == 0) {
    if (circular_buffer_count() > 0) {
      circular_buffer_get(&gActiveState);
      printf("executing command");
      show_command(&gActiveState);

    }
  }
  return (gActiveState.repeat_count == 0 && gQuit);
}



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
    kbd_command_process();

    exit = run_commands();

    const struct SpineMessageHeader* hdr = get_a_frame(10);
    if (!hdr) {
      struct BodyToHead fakeData = {{0}};
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
        printf("got header %x\n", hdr->payload_type);
      }
    }

  }

  on_exit();

  return 0;
}
