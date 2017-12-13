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


struct {
  int repeat_count;
  int16_t motorValues[MOTOR_COUNT];
  uint16_t printmask;
} gCommander = {0};



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
    gCommander.repeat_count = nrepeats;
  }
  return end;
}

const char* handle_motor(const char* text, int len, RobotMotor motor)
{
  char* end;
  double power = strtof(text, &end);
  if (power > 1.0) { power = 1.0;}
  if (power < -1.0) { power = -1.0;}
  printf("Motor %d power = %f (%d%%)\n", motor, power, (int)(power*100));
  if (end!=text && end <= text+len) //converted some chars
  {
    gCommander.motorValues[motor] = 0x7FFF * power;
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
  gCommander.printmask |= (1<<show_ENCODER);
  return text;
}

const char* handle_prox_command(const char* text, int len)
{
  gCommander.printmask |= (1<<show_PROX);
  return text;
}

const char* handle_touch_command(const char* text, int len)
{
  gCommander.printmask |= (1<<show_TOUCH);
  return text;
}

const char* handle_speed_command(const char* text, int len)
{
  gCommander.printmask |= (1<<show_SPEED);
  return text;
}

const char* handle_bat_command(const char* text, int len)
{
  gCommander.printmask |= (1<<show_BAT);
  return text;
}
const char* handle_cliff_command(const char* text, int len)
{
  gCommander.printmask |= (1<<show_CLIFF);
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
  REGISTER_COMMAND(head),
  REGISTER_COMMAND(lift),
  REGISTER_COMMAND(left),
  REGISTER_COMMAND(right),
  REGISTER_COMMAND(enc),
  REGISTER_COMMAND(speed),
  REGISTER_COMMAND(cliff),
  REGISTER_COMMAND(prox),
  REGISTER_COMMAND(touch),
 /* ^^ insert new commands here ^^ */
  {0}
};


int parse_command_text(const char* command, int len)
{

  while (len > 0)  {
    const char* next_word = NULL;
    const CommandHandler* candidate = &handlers[0];

//    printf("HANDLING [ %.*s ] ...", len, command);

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
  return -1;
}

#define LINEBUFSZ 512
int kbd_command_process(void) {
  static int linelen = 0;
  static char linebuf[LINEBUFSZ+1];
  if (kbhit()) {
    int nread = read(0, linebuf+linelen, LINEBUFSZ-linelen);
    if (nread<0) { return 1; }
    printf("%.*s", nread, linebuf+linelen);
    fflush(0);

    char* endl = memchr(linebuf+linelen, '\n', nread);
    if (!endl) {
      linelen+=nread;
      if (linelen >= LINEBUFSZ)
      {
        printf("TOO MANY CHARACTERS, truncating to %d\n", LINEBUFSZ);
        endl = linebuf+LINEBUFSZ-1;
        *endl = '\n';
      }
    }
    if (endl) {
      *endl = '\0';
      parse_command_text(linebuf, endl-linebuf);
      endl++;
      if (endl < linebuf+linelen) {
        linelen -= endl-linebuf;
        memmove(linebuf, endl, linelen);
      }
      else {
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
  while (*contactData) {
    char c = *contactData;
    if (c=='\n') {
      linebuf[linelen]='\0';
      //if has command header, process line
      if (linelen>2 && linebuf[0]=='>' && linebuf[1]=='>')  {
        parse_command_text(linebuf+2, linelen-2);
      }
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
  if (gCommander.repeat_count) {
    int i;
    for (i=0;i<4;i++) {
      gHeadData.motorPower[i] = gCommander.motorValues[i];
    }
  }
  else {
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
    printf("fl_cliff, fr_cliff, br_cliff, bl_cliff ");
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
  if (gCommander.repeat_count) {
    if (gCommander.printmask != oldmask) {
      oldmask = show_legend(gCommander.printmask);
    }

    printf("%d ", bodyData->framecounter);
    if (gCommander.printmask & (1<<show_ENCODER)) {
      printf("%d %d %d %d ",
             bodyData->motor[0].position,
             bodyData->motor[1].position,
             bodyData->motor[2].position,
             bodyData->motor[3].position );
    }
    if (gCommander.printmask & (1<<show_SPEED)) {
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
    if (gCommander.printmask & (1<<show_CLIFF)) {
      printf("%d %d %d %d ",
             bodyData->cliffSense[0],
             bodyData->cliffSense[1],
             bodyData->cliffSense[2],
             bodyData->cliffSense[3] );
    }
    if (gCommander.printmask & (1<<show_BAT)) {
      printf("%d ",
             bodyData->battery.battery);
    }
    if (gCommander.printmask & (1<<show_PROX)) {
      printf("%d",
             bodyData->proximity.rangeMM);
    }
    if (gCommander.printmask & (1<<show_TOUCH)) {
      printf("%d %d",
             bodyData->touchLevel[0],
             bodyData->touchLevel[1]);
    }
    printf("\n");
    if (--gCommander.repeat_count == 0) {
      //clear print request at end
      gCommander.printmask = 0;
    }
  }
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

  //kick off the body frames
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));

  while (!exit)
  {
    exit = kbd_command_process();
    populate_outgoing_frame();
    const struct SpineMessageHeader* hdr = get_a_frame(10);
    if (!hdr) {
      struct BodyToHead fakeData = {0};
      process_incoming_frame(&fakeData);
    }
    else {
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         struct BodyToHead* bodyData = (struct BodyToHead*)(hdr+1);
         process_incoming_frame(bodyData);
         hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));
      }
      else if (hdr->payload_type == PAYLOAD_CONT_DATA) {
        struct ContactData* contactData = (struct ContactData*)(hdr+1);
        gather_contact_text((char*)contactData->data, sizeof(contactData->data));
      }
    }

  }

  on_exit();

  return 0;


}
