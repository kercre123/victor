#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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


void core_common_on_exit(void)
{
  //stop motors and restore charger before shutdown
  memset(&gHeadData, 0, sizeof(gHeadData));
  gHeadData.powerFlags = POWER_CONNECT_CHARGER;
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));
  //TODO: hal_terminate();
}


void safe_quit(int n)
{
  error_exit(app_USAGE, "Caught signal %d \n", n);
  exit(1);
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

#define MIN_SELECT_HIGHLIGHT_CYCLES 10

void menu_select()
{
  if (!gMenu.is_busy) {
    gMenu.changed = 1;
    if (gMenu.current->item[gMenu.index].handler)
    {
      gMenu.selection_count = MIN_SELECT_HIGHLIGHT_CYCLES;
      gMenu.current->item[gMenu.index].handler(gMenu.current->item[gMenu.index].param);
     }
    if (gMenu.current->item[gMenu.index].submenu)
    {
      gMenu.current = gMenu.current->item[gMenu.index].submenu;
      gMenu.index = 0;
    }
  }
}

bool menu_jump(int opt) { //returns true if item selected
  if (!gMenu.is_busy) {
    if (opt >= 0 && opt < gMenu.current->nItems) {
      printf("doing menu option %d\n",opt);
      bool isAction = (gMenu.current->item[opt].handler != NULL);
      gMenu.selection_count = 0;
      gMenu.index = opt;
      menu_select();
      return isAction;
    }
  }
  return false;
}


void menu_set_busy(int isBusy) {
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


typedef struct Process_t {
  pid_t wait_pid;
  int proc_fd;
} Process;
static Process gProcess;

void wait_for_process(pid_t pid, int fd)
{
  gProcess.wait_pid = pid;
  gProcess.proc_fd = fd;
  mm_debug_x("waiting for process\n");
  menu_set_busy(1);
}


void process_monitor(Process* proc){
  if (proc->wait_pid)
  {
    int status;

    if (wait_for_data(proc->proc_fd, 0)) {
      char buffer[512];
      int n = read(proc->proc_fd, buffer, 512);
      if (n>0) {
        printf("%.*s", n, buffer);
      }
    }

    if (waitpid(proc->wait_pid, &status, WNOHANG)) {
      proc->wait_pid = 0;
      mm_debug_x("proc finished\n");
      menu_set_busy(0);
    }
  }
}




#define MOTOR_TEST_POWER 0x3FFF
#define MOTOR_TEST_DROPS  50

uint64_t gMotorTestCycles = 0;

void start_motor_test(bool lockMenus)
{
  if (!gMotorTestCycles) {
    gMotorTestCycles =1 ;
  }
  mm_debug_x("starting motor test\n");
  menu_set_busy(lockMenus);
}

//returns true if actually stopped
int stop_motor_test(void)
{
  if (gMotorTestCycles) {
    gMotorTestCycles = 0;
    menu_set_busy(0);
    return 1;
  }
  return 0;
}

void command_motors(struct HeadToBody* data)
{
  static int drop_count = 0;
  if (gMotorTestCycles) {
    int i;
    int16_t power = (gMotorTestCycles%2) ? MOTOR_TEST_POWER : -MOTOR_TEST_POWER;
    for (i=0;i<MOTOR_COUNT;i++) {
      data->motorPower[i] = power * (1 -2*(i%2));
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


#define CHARGE_DISABLE_PERIOD 40 // 200ms

typedef struct lifeTester_ {
  /* types of tests */
  bool battery;
  bool cpu;
  bool flash;
  bool motors;
  bool screen;
  /* status */
  bool do_discharge;
  Process burn_proc;
} LifeTester;

LifeTester  gLifeTesting;


#define LCD_FRAME_RATE 20  //1/10th second
void screen_test(void)
{
  static LcdFrame frame={{0}};
  static uint16_t update_period = LCD_FRAME_RATE;
  static uint16_t color = 0;
  static uint16_t row = 0;
  if (--update_period == 0) {
    update_period = LCD_FRAME_RATE;
    int i;
    for (i=0;i<LCD_FRAME_WIDTH;i++) {
      frame.data[i+row*LCD_FRAME_WIDTH] = color++;
    }
    if (++row > LCD_FRAME_HEIGHT) {row=0;}
    lcd_draw_frame(&frame);
  }
}


void manage_life_tests() {
  if (gLifeTesting.flash) {
  }
  if (gLifeTesting.motors ) {
    if (!gMotorTestCycles) {
      start_motor_test(false);
    }
  }
  if (gLifeTesting.screen) {
    screen_test();
  }
  if (gLifeTesting.cpu) {
    //print any output;
    process_monitor(&gLifeTesting.burn_proc);
  }
}

#define BATTERY_SCALE (2.8/2048)
#define BAT_CHARGED_THRESHOLD ((uint16_t)(4.1 / BATTERY_SCALE))
#define BAT_DISCHARGED_THRESHOLD ((uint16_t)(3.6 / BATTERY_SCALE))

void check_battery_discharge(struct BodyToHead*  bodyData) {
  static uint32_t charge_cycles = 0;
  if (gLifeTesting.battery) {
    charge_cycles++;

    if ((charge_cycles & 0x7F)==0) { printf("battery is %.2fV\r", bodyData->battery.main_voltage * BATTERY_SCALE); }
    if ((charge_cycles & 0x1FF)==0) {
      mm_debug("battery is %.2fV (%d) [ext %d, temp %d flags %d]. We are %scharging",
               bodyData->battery.main_voltage * BATTERY_SCALE,
               bodyData->battery.main_voltage,
               bodyData->battery.charger,
               bodyData->battery.temperature,
               bodyData->battery.flags,
               gLifeTesting.do_discharge?"dis":""); }

    if (bodyData->battery.main_voltage > BAT_CHARGED_THRESHOLD) {
      if (!gLifeTesting.do_discharge) {
        mm_debug("Battery charged for %d cycles to %fV", charge_cycles,
                 bodyData->battery.main_voltage * BATTERY_SCALE);
        gLifeTesting.do_discharge = true;

        charge_cycles = 0;
      }
    }
    if (bodyData->battery.main_voltage < BAT_DISCHARGED_THRESHOLD) {
      if (gLifeTesting.do_discharge) {
        mm_debug("Battery discharged for %d cycles to %fV", charge_cycles,
                 bodyData->battery.main_voltage * BATTERY_SCALE);
        gLifeTesting.do_discharge = false;
        charge_cycles = 0;
      }
    }
  }
}


/****  Menu Handlers ****/


void handle_MainBurn(void* param) {
  printf("Burn Selected\n");
  pid_t pidout;
  int fd = pidopen("./burn.sh", "", &pidout);
  wait_for_process(pidout, fd);
}


void handle_MainCubeTest(void* param) {
  printf("CubeTest Selected\n");
  pid_t pidout;
  pidopen("./cubetest.sh", "", &pidout);
}

void handle_MainMotorTest(void* param) {
  printf("MotorTest Selected\n");
  start_motor_test(true);
}

void handle_SystemCall(void* param) {
  pid_t pidout;
  printf(">> system.sh %s\n", (char*)param);
  int fd = pidopen("./system.sh", (char*)param, &pidout);
  wait_for_process(pidout, fd);
}


void handle_LifeTestBattery(void* param)
{
  printf("Battery Cycle Selected\n");
  gLifeTesting.battery = true;
  gLifeTesting.do_discharge = true;
}

void handle_LifeTestCpu(void* param)
{
  printf("CPU/Flash Test Selected\n");
  gLifeTesting.burn_proc.proc_fd = pidopen("./burn.sh", "", &gLifeTesting.burn_proc.wait_pid);
  gLifeTesting.cpu = true;
}

void handle_LifeTestMotors(void* param)
{
  printf("Motor Life Test Selected\n");
  gLifeTesting.motors = true;
}
void handle_LifeTestScreen(void* param)
{
  printf("Screen Test Selected\n");
  gLifeTesting.screen = true;
}

void handle_LifeTestAll(void* param) {
  handle_LifeTestBattery(param);
  handle_LifeTestCpu(param);
  handle_LifeTestMotors(param);
  handle_LifeTestScreen(param);
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

MENU_ITEMS(LifeTest)= {
  MENU_BACK(Main),
  MENU_ACTION("Cycle Battery", LifeTestBattery),
  MENU_ACTION("CPU / Flash ", LifeTestCpu),
  MENU_ACTION("Run Motors", LifeTestMotors),
  MENU_ACTION("Cycle Screens", LifeTestScreen),
  MENU_ACTION("ALL", LifeTestAll),
};
MENU_CREATE(LifeTest);

MENU_ITEMS(Main) = {
  MENU_SUBMENU("TX Carrier", TXCW),
  MENU_SUBMENU("TX 11n", TXN),
  MENU_SUBMENU("TX 11g", TXG),
  MENU_SUBMENU("TX 11b", TXB),
  MENU_ACTION("Burn CPU", MainBurn ),
  MENU_ACTION("Cube Test", MainCubeTest ),
  MENU_ACTION("Motor Test", MainMotorTest ),
  MENU_SUBMENU("Life Testing", LifeTest),
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
    command_motors(&gHeadData);
  }
  else {
    //make sure motors are off
    memset(gHeadData.motorPower, 0, sizeof(gHeadData.motorPower));
  }
  if (gLifeTesting.battery) {
    if (gLifeTesting.do_discharge && !(gHeadData.powerFlags & POWER_DISCONNECT_CHARGER))
    {
      printf("Setting POWER_DISCONNECT_CHARGER flag\n");
    }
    else if (!gLifeTesting.do_discharge && !(gHeadData.powerFlags & POWER_CONNECT_CHARGER))
    {
      printf("Setting POWER_CONNECT_CHARGER flag\n");
    }
    gHeadData.powerFlags = gLifeTesting.do_discharge ? POWER_DISCONNECT_CHARGER : POWER_CONNECT_CHARGER;
  }

}

#define MENU_TREAD_VELOCITY_THRESHOLD 1
#define MENU_TREAD_TRIGGER_DELTA       120
#define MENU_LIFT_TRIGGER_DELTA       75
#define INITIAL_LIFT_POS 0x0FFFffff

void process_menu_controls(struct BodyToHead* bodyData)
{
  static int32_t liftMinPosition = INITIAL_LIFT_POS;
  static int32_t treadLastPosition = 0;
  static bool buttonPressed = false;

  if (liftMinPosition == INITIAL_LIFT_POS) {
    liftMinPosition = bodyData->motor[MOTOR_LIFT].position;
    treadLastPosition = bodyData->motor[MOTOR_RIGHT].position;
  }

  if (!gLifeTesting.motors) {

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

  if (bodyData->touchLevel[1] > 0 )  {
    if (!buttonPressed) {
      buttonPressed = true;
      if (!stop_motor_test()) {
        menu_select();
      }
      else {
        gLifeTesting.motors=0;
        gLifeTesting.screen=0;
      }
    }
  }
  else {
    buttonPressed = false;
  }

}


void process_incoming_frame(struct BodyToHead* bodyData)
{
  process_menu_controls(bodyData);
  check_battery_discharge(bodyData);
}


int main(int argc, const char* argv[])
{
  bool shouldexit = false;
  int badcomms = 0;

  signal(SIGINT, safe_quit);
  signal(SIGKILL, safe_quit);

  lcd_init();
  lcd_set_brightness(5);
  display_init();

  menus_init();

  int errCode = hal_init(SPINE_TTY, SPINE_BAUD);
  if (errCode) { error_exit(errCode, "hal_init"); }

//  hal_set_mode(RobotMode_RUN);

  //kick off the body frames
  hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));

  /* usleep(5000); */
  /* hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData)); */


  while (--argc>0) {
    ++argv;
    char *endptr;
    long option = strtol(*argv, &endptr, 10);
    if (endptr != *argv && *endptr == '\0') {
      if (menu_jump(option -1 )) { //1st item is 0th entry
        break;
      }
      draw_menus();
    }
  }


  while (!shouldexit)
  {
    draw_menus();
    process_monitor(&gProcess);

    const struct SpineMessageHeader* hdr = get_a_frame(5);
    if (!hdr) {
      if (++badcomms > 1000)
      {
        printf("can't get frame\n");
        exit(1);
      }
    }
    else {
      badcomms = 0;
      if (hdr->payload_type == PAYLOAD_DATA_FRAME) {
         struct BodyToHead* bodyData = (struct BodyToHead*)(hdr+1);
         process_incoming_frame(bodyData);
         manage_life_tests();
         populate_outgoing_frame();
         hal_send_frame(PAYLOAD_DATA_FRAME, &gHeadData, sizeof(gHeadData));
      }
      else if (hdr->payload_type == PAYLOAD_BOOT_FRAME) {
        mm_debug("got unexpected header %x\n", hdr->payload_type);
        hal_send_frame(PAYLOAD_MODE_CHANGE, NULL, 0);
      }
      else {
        mm_debug("got unexpected header %x\n", hdr->payload_type);
      }
    }
  }

  core_common_on_exit();

  return 0;
}
