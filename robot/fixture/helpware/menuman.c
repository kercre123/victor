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

#define MENU_TEXT_COLOR_FG lcd_WHITE
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


typedef void (*MenuActionHandler)(void* param);

typedef struct MenuItem_t {
  const char* text;
  const MenuActionHandler handler;
  const struct MenuLevel_t* submenu;
  void* param;
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
  int is_busy;
} gMenu;


/* Menuing */


void menu_advance()
{
  if (!gMenu.is_busy) {
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
  if (!gMenu.is_busy) {
    gMenu.changed = 1;
    if (gMenu.current->item[gMenu.index].handler)
    {
      gMenu.selection_count = 10;
      gMenu.current->item[gMenu.index].handler(gMenu.current->item[gMenu.index].param);
     }
    if (gMenu.current->item[gMenu.index].submenu)
    {
      gMenu.current = gMenu.current->item[gMenu.index].submenu;
      gMenu.index = 0;
    }
  }
}


void menu_set_busy(int isBusy) {
  printf("menu is%sbusy\n", isBusy?" ":"n't ");
  gMenu.is_busy = isBusy;
  gMenu.changed = 1;
}


int menu_get_index(void) {
  return gMenu.index;
}

void draw_menus(void) {
  int i;
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
      if (gMenu.selection_count || gMenu.is_busy) {
        if (i==gMenu.index) { inversion = MENU_TEXT_COLOR_FG; }
      }
      display_draw_text(DISPLAY_LAYER_SMALL, i,
                        MENU_TEXT_COLOR_FG^inversion, MENU_TEXT_COLOR_BG^inversion,
                        textbuffer, sizeof(textbuffer), false);
    }
    display_render(0xFF);  //1<<DISPLAY_LAYER_SMALL);
  }
}


static struct {
  pid_t wait_pid;
  int proc_fd;
} gProcess;

void wait_for_process(pid_t pid, int fd)
{
  gProcess.wait_pid = pid;
  gProcess.proc_fd = fd;
  menu_set_busy(1);
}


void process_monitor(void){
  if (gProcess.wait_pid)
  {

    int status;

    if (wait_for_data(gProcess.proc_fd, 0)) {
      char buffer[512];
      int n = read(gProcess.proc_fd, buffer, 512);
      if (n>0) {
        printf("%.*s", n, buffer);
      }
    }

    if (waitpid(gProcess.wait_pid, &status, WNOHANG)) {
      gProcess.wait_pid = 0;
      printf("proc finished\n");
      menu_set_busy(0);
    }
  }
}




#define MOTOR_TEST_POWER 0x3FFF
#define MOTOR_TEST_DROPS  50

uint64_t gMotorTestCycles = 0;

void start_motor_test(void)
{
  gMotorTestCycles =1 ;
  menu_set_busy(1);
}

void stop_motor_test(void)
{
  gMotorTestCycles = 0;
  menu_set_busy(0);
}

void command_motors(struct HeadToBody* data)
{
  static int drop_count = 0;
  if (gMotorTestCycles) {
    int i;
    int16_t power = (gMotorTestCycles%2) ? MOTOR_TEST_POWER : -MOTOR_TEST_POWER;
    for (i=0;i<MOTOR_COUNT;i++) {
      data->motorPower[i] = power;
    }
    if (drop_count)
    {
      drop_count--;
    }
    else {
      gMotorTestCycles++;
      drop_count = MOTOR_TEST_DROPS;
    }
  }
}


/****  Menu Handlers ****/


void handle_MainPlaySound(void* param) {
  printf("PlaySound Selected\n");
  pid_t pidout;
  int fd = pidopen("./testsound.sh", "test.wav", &pidout);
  wait_for_process(pidout, fd);
}

void handle_MainMotorTest(void* param) {
  printf("MotorTest Selected\n");
  start_motor_test();
}

void handle_SystemCall(void* param) {
  pid_t pidout;
  printf(">> system.sh %s\n", (char*)param);
  int fd = pidopen("./system.sh", (char*)param, &pidout);
  wait_for_process(pidout, fd);
}




/************* MENU DEFINITIONS **************/

#define MENU_ITEMS(name) static const MenuItem menu_items_ ## name []
#define MENU_ACTION(text, tag) { text, handle_ ## tag, NULL, NULL }
#define MENU_SHELL(text, param) { text, handle_SystemCall, NULL, param }
#define MENU_SUBMENU(text, name) { text, NULL, &menu_ ## name, NULL }
#define MENU_BACK(parent) { "Back", NULL, &menu_ ## parent }

#define MENU_CREATE(name) MenuLevel menu_ ## name =  { menu_items_ ## name, sizeof(menu_items_ ## name)/sizeof(MenuItem) }

extern MenuLevel menu_Main;

MENU_ITEMS(TXCW) = {
  MENU_BACK(Main),
  MENU_SHELL("STOP TX", "STOP"),
  MENU_SHELL("Carrier @ 1  2412MHz", "CW 1"),
  MENU_SHELL("Carrier @ 6  2437MHz", "CW 6"),
  MENU_SHELL("Carrier @ 11 2462MHz", "CW 11"),
};
MENU_CREATE(TXCW);

MENU_ITEMS(TXN) = {
  MENU_BACK(Main),
  MENU_SHELL("STOP TX", "STOP"),
  MENU_SHELL("11n @ 1  2412MHz", "MCS_65_MBPS 1"),
  MENU_SHELL("11n @ 6  2437MHz", "MCS_65_MBPS 6"),
  MENU_SHELL("11n @ 11 2462MHz", "MCS_65_MBPS 11"),
};
MENU_CREATE(TXN);

MENU_ITEMS(TXG) = {
  MENU_BACK(Main),
  MENU_SHELL("STOP TX", "STOP"),
  MENU_SHELL("11g @ 1  2412MHz", "11A_54_MBPS 1"),
  MENU_SHELL("11g @ 6  2437MHz", "11A_54_MBPS 6"),
  MENU_SHELL("11g @ 11 2462MHz", "11A_54_MBPS 11"),
};
MENU_CREATE(TXG);

MENU_ITEMS(TXB) = {
  MENU_BACK(Main),
  MENU_SHELL("STOP TX", "STOP"),
  MENU_SHELL("11b @ 1  2412MHz", "11B_LONG_11_MBPS 1"),
  MENU_SHELL("11b @ 6  2437MHz", "11B_LONG_11_MBPS 6"),
  MENU_SHELL("11b @ 11 2462MHz", "11B_LONG_11_MBPS 11"),
};
MENU_CREATE(TXB);

MENU_ITEMS(Main) = {
  MENU_SUBMENU("TX Carrier", TXCW),
  MENU_SUBMENU("TX 11n", TXN),
  MENU_SUBMENU("TX 11g", TXG),
  MENU_SUBMENU("TX 11b", TXB),
  MENU_ACTION("Burn CPU", MainPlaySound ),
  MENU_ACTION("Play Sound", MainPlaySound ),
  MENU_ACTION("Motor Test", MainMotorTest ),
};
MENU_CREATE(Main);


void menus_init() {
  gMenu.selection_count = 0;
  gMenu.current = &menu_Main;
  gMenu.index = 0;
  gMenu.changed = 1;
  gMenu.is_busy = 0;
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
  if (gMotorTestCycles) { //if there are conditions to communicate to body, put them here
//    printf("commanding motors\n");
    command_motors(&gHeadData);
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
  static bool buttonPressed = false;
  /* static uint8_t filter; */

  if (liftMinPosition == INITIAL_LIFT_POS) {
    liftMinPosition = bodyData->motor[MOTOR_LIFT].position;
    treadLastPosition = bodyData->motor[MOTOR_RIGHT].position;
  }
  /* if (!((filter++)&0x3F)) { */
    /* mm_debug_x("%d / %d : %d / %d\n", */
    /*        bodyData->motor[MOTOR_RIGHT].position, treadLastPosition, */
    /*        bodyData->motor[MOTOR_LIFT].position, liftMinPosition); */
  /*   mm_debug("[%d, %d]  was %d\n", bodyData->touchLevel[0], bodyData->touchLevel[1], buttonPressed); */
  /* } */

  if (abs(bodyData->motor[MOTOR_RIGHT].position - treadLastPosition) > MENU_TREAD_TRIGGER_DELTA )
  {
    treadLastPosition = bodyData->motor[MOTOR_RIGHT].position;
    menu_advance();
  }
  if (bodyData->touchLevel[1] > 0 )  {
    if (!buttonPressed) {
      printf("button pressed\n");
      buttonPressed = true;
      menu_select();
      stop_motor_test();
    }
  }
  else {
    buttonPressed = false;
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
    process_monitor();

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
