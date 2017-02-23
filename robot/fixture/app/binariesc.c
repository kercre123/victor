#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "hal/monitor.h"
#include "hal/portable.h"
#include "hal/timers.h"
#include "hal/uart.h"
#include "hal/console.h"
#include "app/app.h"
#include "app/binaries.h"

extern int g_canary;
bool g_format_csv;

//print info in csv compatible format
#define FORMAT_CSV  1

//FCC switch
#ifdef FCC
#define BINFCC 1
#else
#define BINFCC 0
#endif

//Hardware Version is hard coded into the body bootloader:
//  syscon/boot/startup_nrf51.s
//  DCD  # ;Syscon version - 0 = pre-Pilot, 1 = Pilot, 3 = "First 1000" Prod, 4 = Full Prod, 5 = 1.5 EP2
#define BODY_BOOT_BIN_HW_VER_ADDR   0x00010 /*absolute 0x1F010*/
static u32 _bodyBootHwId(void) {
  return (g_BodyBootEnd-g_BodyBoot) >= BODY_BOOT_BIN_HW_VER_ADDR+3 ? *((uint32_t*)&g_BodyBoot[BODY_BOOT_BIN_HW_VER_ADDR]) : 0xFFFFffff;
}

static char* _bodyBootHwIdVers(char* s) {
  sprintf(s, "HW:%d", _bodyBootHwId() );
  return s;
}

static char* _bodyBootHwIdDebug(char* out_s)
{
  char *s;
  switch( _bodyBootHwId() ) {
    case 0: s = (char*)"pre-pilot"; break;
    case 1: s = (char*)"pilot"; break;
    case 2: s = (char*)"reserved"; break;
    case 3: s = (char*)"First 1000 Prod"; break;
    case 4: s = (char*)"Full Prod"; break;
    case 5: s = (char*)"1.5 EP2"; break;
    default:s = (char*)"INVALID"; break;
  }
  return s;
}

//column separation. padded ' ' for screen, packed commas for CSV output
static inline void _column_pad(int len, int min_len) {
  if( g_format_csv )
    ConsoleWrite((char*)",");
  else {
    for( ; len < min_len; len++)
      ConsoleWrite((char*)" ");
  }
}

//Print a row of the binary table. Pads column widths to known sizes.
static void _bin_print(char *name, char *file, int size, char *version, char *debug)
{
  if( !g_format_csv )
    ConsolePrintf("  ");
  
  int len = ConsolePrintf(name);
  _column_pad(len, 12);
  
  len = ConsolePrintf(file);
  _column_pad(len, 16);
  
  len = (size > 0) ? ConsolePrintf("0x%06X", size) : size == 0 ? ConsolePrintf("0") : ConsolePrintf("SIZE");
  _column_pad(len, 10);
  
  len = (version != NULL) ? ConsolePrintf(version) : ConsolePrintf("na");
  _column_pad(len, 15);
  
  len = (debug != NULL) ? ConsolePrintf(debug) : 0;
  _column_pad(len, 0);
  
  ConsoleWrite((char*)"\r\n");
}

void binPrintInfo(bool csv)
{
  g_format_csv = csv;
  char str[20]; //temp string for inline parsing
  
  //print title row for console view
  //if( !g_format_csv )
    _bin_print((char*)"NAME",   (char*)"BIN-FILE",        -1,                                 (char*)"VERSION",       (char*)"DEBUG");
  _bin_print((char*)"Cube",     (char*)"xsboot.bin",      BINFCC ? 0 : g_CubeEnd-g_Cube,      (char*)"?",             NULL);
  _bin_print((char*)"Cube-FCC", (char*)"xsfcc.bin",       BINFCC ? g_CubeEnd-g_Cube : 0,      (char*)"?",             NULL);
  _bin_print((char*)"Body",     (char*)"syscon.bin",      g_BodyEnd-g_Body,                   NULL,                   NULL);
  _bin_print((char*)"BodyBoot", (char*)"sys_boot.bin",    g_BodyBootEnd-g_BodyBoot,           _bodyBootHwIdVers(str), _bodyBootHwIdDebug(str));
  _bin_print((char*)"BodyBLE",  (char*)"s110~.bin",       g_BodyBLEEnd-g_BodyBLE,             (char*)"?",             NULL);
  _bin_print((char*)"BodyStub", (char*)"nrf51_stub.bin",  g_stubBodyEnd-g_stubBody,           (char*)"?",             NULL);
  _bin_print((char*)"K02",      (char*)"robot.bin",       BINFCC ? 0 : g_K02End-g_K02,        (char*)"?",             NULL);
  _bin_print((char*)"K02-FCC",  (char*)"robot.fcc.bin",   BINFCC ? g_K02End-g_K02 : 0,        (char*)"?",             NULL);
  _bin_print((char*)"K02Boot",  (char*)"robot_boot.bin",  g_K02BootEnd-g_K02Boot,             (char*)"?",             NULL);
  _bin_print((char*)"K02Stub",  (char*)"k02_stub.bin",    g_stubK02End-g_stubK02,             (char*)"?",             NULL);
  _bin_print((char*)"ESP",      (char*)"esp.factory.bin", BINFCC ? 0 : g_EspUserEnd-g_EspUser,(char*)"?",             NULL);
  _bin_print((char*)"ESP-FCC",  (char*)"esp.fcc.bin",     BINFCC ? g_EspUserEnd-g_EspUser : 0,(char*)"?",             NULL);
  #if BINFCC < 1
  _bin_print((char*)"ESPBoot",  (char*)"esp.boot.bin",    g_EspBootEnd-g_EspBoot,             (char*)"?",         NULL);
  _bin_print((char*)"ESPInit",  (char*)"esp.init.bin",    g_EspInitEnd-g_EspInit,             (char*)"?",         NULL);
  _bin_print((char*)"ESPSafe",  (char*)"esp.safe.bin",    g_EspSafeEnd-g_EspSafe,             (char*)"?",         NULL);
  #endif
  _bin_print((char*)"Fix.Radio",(char*)"radio.bin",       g_RadioEnd-g_Radio,                 (char*)"?",         NULL);

  sprintf(str,"0x%08X",g_canary);
  _bin_print((char*)"Canary",   str,                      0,                                  NULL,         NULL);
}

