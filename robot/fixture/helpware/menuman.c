#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "core/common.h"
#include "core/lcd.h"
#include "core/clock.h"
#include "helpware/display.h"

#include "spine/spine_hal.h"
#include "schema/messages.h"

#include "pidopen.h"

#define mm_debug(fmt, args...)  (LOGD( fmt, ##args))

#define EXTENDED_MM_DEBUG 1
#if EXTENDED_MM_DEBUG
#define mm_debug_x mm_debug
#else
#define mm_debug_x(fmt, args...)
#endif

#define MENU_TEXT_COLOR_FG lcd_BLUE
#define MENU_TEXT_COLOR_BG lcd_BLACK

const struct SpineMessageHeader* hal_read_frame();
struct HeadToBody gHeadData = {0};


void on_exit(void)
{
  //TODO: hal_terminate();
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
}


typedef void (*MenuActionHandler)(void);

typedef struct MenuItem_t {
  const char* text;
  const MenuActionHandler handler;
  const struct MenuLevel_t* submenu;
} MenuItem;

typedef struct MenuLevel_t {
  const MenuItem* item;
  int nItems;
} MenuLevel;

struct MenuManager_t {
  int index;
  int selection_count;
  const MenuLevel* current;
  int changed;
  pid_t busy_pid;
} gMenu;


/* Menuing */

void wait_for_process(pid_t pid)
{
  gMenu.busy_pid = pid;
}

void menu_advance()
{
  if (!gMenu.busy_pid) {
    gMenu.selection_count = 0;
    gMenu.index++;
    gMenu.changed = 1;

    if (gMenu.index >= gMenu.current->nItems) {
      gMenu.index = 0;
    }
  }
}

void menu_select()
{
  if (!gMenu.busy_pid) {
    gMenu.changed = 1;
    if (gMenu.current->item[gMenu.index].handler)
    {
      gMenu.selection_count = 10;
      gMenu.current->item[gMenu.index].handler();
      printf("Handled!\n");
    }
    if (gMenu.current->item[gMenu.index].submenu)
    {
      gMenu.current = gMenu.current->item[gMenu.index].submenu;
      gMenu.index = 0;
    }
  }
}


void draw_menus(void) {
  int i;
  if (gMenu.busy_pid)
  {
    int status;
    if (waitpid(gMenu.busy_pid, &status, WNOHANG)) {
      gMenu.busy_pid = 0;
      gMenu.changed = 1;
    }
  }
  if (gMenu.selection_count) {
    if (--gMenu.selection_count == 0)
      gMenu.changed = 1;

  }
  if (gMenu.changed) {
    gMenu.changed = 0;
    display_clear_layer(DISPLAY_LAYER_SMALL,
                        MENU_TEXT_COLOR_FG, MENU_TEXT_COLOR_BG);
    for (i=0;i< gMenu.current->nItems; i++) {
      char textbuffer[20];
      uint16_t inversion = 0;
      snprintf(textbuffer, sizeof(textbuffer), "%c %s",
               (i==gMenu.index)?'>':' ', gMenu.current->item[i].text);
      if (gMenu.selection_count || gMenu.busy_pid) {
        if (i==gMenu.index) { inversion = MENU_TEXT_COLOR_FG; }
      }
      display_draw_text(DISPLAY_LAYER_SMALL, i,
                        MENU_TEXT_COLOR_FG^inversion, MENU_TEXT_COLOR_BG^inversion,
                        textbuffer, sizeof(textbuffer), false);
    }
    display_render(0xFF);  //1<<DISPLAY_LAYER_SMALL);
  }
}



int shellcommand(int timeout_sec, const char* command, const char* argstr) {
  int retval = -666;
  uint64_t expiration = steady_clock_now()+(timeout_sec*NSEC_PER_SEC);

  int pid;
  int pfd = pidopen(command, argstr, &pid);
  bool timedout = false;

  if (pfd>0) {
    uint64_t scnow;// = steady_clock_now();
    char buffer[512];
    int n;
    do {
      printf("Waiting\n");
      if (wait_for_data(pfd, 0)) {
        printf("Reading\n");
        n = read(pfd, buffer, 512);
        if (n>0) {
          printf("%.*s", n, buffer);

        }
      }
      scnow = steady_clock_now();
      printf("comparing %lld >? %lld\n", scnow, expiration);
      if (scnow > expiration) {
        printf("TIMEOUT after %d sec\n", timeout_sec);
        timedout = true;
        break;
      }
    } while (n>0 );
    printf("closing?\n");
    retval = pidclose(pid, timedout);
  }

  return retval;
}


/****  Menu Handlers ****/


void handle_MainPlaySound(void) {
  printf("PlaySound Selected\n");
  pid_t pidout;
  pidopen("/usr/bin/tinyplay", "test.wav", &pidout);
  wait_for_process(pidout);
}

void handle_DetailsNoOp(void) {
  printf("NoOp Selected\n");
}



/************* MENU DEFINITIONS **************/

#define MENU_ITEMS(name) static const MenuItem menu_items_ ## name []
#define MENU_ACTION(text, tag) { text, handle_ ## tag, NULL }
#define MENU_SUBMENU(text, name) { text, NULL, &menu_ ## name }
#define MENU_BACK(parent) { "Back", NULL, &menu_ ## parent }

#define MENU_CREATE(name) MenuLevel menu_ ## name =  { menu_items_ ## name, sizeof(menu_items_ ## name)/sizeof(MenuItem) }

extern MenuLevel menu_Main;

MENU_ITEMS(Details) = {
  MENU_ACTION("NoOp", DetailsNoOp ),
  MENU_BACK(Main)
};
MENU_CREATE(Details);

MENU_ITEMS(Main) = {
  MENU_ACTION("PlaySound", MainPlaySound ),
  MENU_SUBMENU("See Details", Details)
};
MENU_CREATE(Main);


void menus_init() {
  gMenu.selection_count = 0;
  gMenu.current = &menu_Main;
  gMenu.index = 0;
  gMenu.changed = 1;
}


/*** Spine management  ***/


const void* get_a_frame(int32_t timeout_ms) {
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
  if (false) { //if there are conditions to communicate to body, put them here
  }
  else {
    //make sure motors are off
    memset(gHeadData.motorPower, 0, sizeof(gHeadData.motorPower));
  }
}

static const float HAL_SEC_PER_TICK = (1.0 / 256) / 48000000;

#define MENU_TREAD_VELOCITY_THRESHOLD 1
#define MENU_TREAD_TRIGGER_DELTA       120
#define MENU_LIFT_TRIGGER_DELTA       75


#define INITIAL_LIFT_POS 0x0FFFffff

void process_incoming_frame(struct BodyToHead* bodyData)
{
  static int32_t liftMinPosition = INITIAL_LIFT_POS;
  static int32_t treadLastPosition = 0;
  static uint8_t filter;

  if (liftMinPosition == INITIAL_LIFT_POS) {
    liftMinPosition = bodyData->motor[MOTOR_LIFT].position;
    treadLastPosition = bodyData->motor[MOTOR_RIGHT].position;
  }
  if (!((filter++)&0x3F)) {
    mm_debug_x("%d / %d : %d / %d\n",
           bodyData->motor[MOTOR_RIGHT].position, treadLastPosition,
           bodyData->motor[MOTOR_LIFT].position, liftMinPosition);
  }

  if (abs(bodyData->motor[MOTOR_RIGHT].position - treadLastPosition) > MENU_TREAD_TRIGGER_DELTA )
  {
    treadLastPosition = bodyData->motor[MOTOR_RIGHT].position;
    menu_advance();
  }
  if (bodyData->motor[MOTOR_LIFT].position - liftMinPosition > MENU_LIFT_TRIGGER_DELTA)
  {
    menu_select();
    liftMinPosition = bodyData->motor[MOTOR_LIFT].position;
  }
  else if (bodyData->motor[MOTOR_LIFT].position < liftMinPosition)
  {
    liftMinPosition = bodyData->motor[MOTOR_LIFT].position;
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

  menus_init();

  int errCode = hal_init(SPINE_TTY, SPINE_BAUD);
  if (errCode) { error_exit(errCode, "hal_init"); }

  hal_set_mode(RobotMode_RUN);

  //kick off the body frames
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));

  usleep(5000);
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));


  while (!exit)
  {
    draw_menus();

    const struct SpineMessageHeader* hdr = get_a_frame(5);
    if (!hdr) {
      /* printf("miss\n"); */
      /* struct BodyToHead fakeData = {0}; */
      /* process_incoming_frame(&fakeData); */
    }
    else {
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         struct BodyToHead* bodyData = (struct BodyToHead*)(hdr+1);
         process_incoming_frame(bodyData);
         populate_outgoing_frame();
         hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));
      }
      else {
        mm_debug("got unexpected header %x\n", hdr->payload_type);
      }
    }
  }

  on_exit();

  return 0;
}
