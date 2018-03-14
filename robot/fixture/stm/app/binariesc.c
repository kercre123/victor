#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "portable.h"
#include "timer.h"
#include "console.h"
#include "app.h"
#include "binaries.h"

extern int g_canary;
bool g_format_csv;

//Body Bootloader Version:
#define BODY_BOOT_BIN_HW_VER_ADDR   0x00010 /*absolute 0x1F010*/
static u32 bodyBootHwId(void) {
  return (g_BodyBootEnd-g_BodyBoot) >= BODY_BOOT_BIN_HW_VER_ADDR+3 ? *((uint32_t*)&g_BodyBoot[BODY_BOOT_BIN_HW_VER_ADDR]) : 0xFFFFffff;
}
static char* bodyBootHwId_Vers(char* out_s) {
  sprintf(out_s, "HW:%d", bodyBootHwId() );
  return out_s;
}
static char* bodyBootHwId_Debug(char* out_s) {
  char *s;
  switch( bodyBootHwId() ) {
    //case 0: s = (char*)"pre-pilot"; break;
    //case 1: s = (char*)"pilot"; break;
    //case 2: s = (char*)"reserved"; break;
    //case 3: s = (char*)"First 1000 Prod"; break;
    case 4: s = (char*)"Full Prod"; break;
    case 5: s = (char*)"1.5 EP2"; break;
    case 6: s = (char*)"1.5c MP"; break;
    default:s = (char*)"INVALID"; break;
  }
  return s;
}

//column separation. padded ' ' for screen, packed commas for CSV output
static inline void column_pad_(int len, int min_len) {
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
  column_pad_(ConsolePrintf(name), 12);
  column_pad_(ConsolePrintf(file), 16);
  int len = (size > 0) ? ConsolePrintf("0x%06X", size) : size == 0 ? ConsolePrintf("0") : ConsolePrintf("SIZE");
  column_pad_(len, 10); 
  len = (version != NULL) ? ConsolePrintf(version) : ConsolePrintf("na");
  column_pad_(len, 15);
  len = (debug != NULL) ? ConsolePrintf(debug) : 0;
  column_pad_(len, 0);
  ConsoleWrite((char*)"\n");
}

void binPrintInfo(bool csv)
{
  g_format_csv = csv;
  char str[20]; //temp string for inline parsing
  
  //print title row for console view
  _bin_print((char*)"NAME",     (char*)"BIN-FILE",          -1,                         (char*)"VERSION",       (char*)"DEBUG");
  _bin_print((char*)"CubeTest", (char*)"cubetest.bin",      g_CubeTestEnd-g_CubeTest,   (char*)"?",             NULL);
  _bin_print((char*)"CubeStub", (char*)"cubeotp.bin",       g_CubeStubEnd-g_CubeStub,   NULL,                   NULL);
  _bin_print((char*)"BodyTest", (char*)"systest.bin",       g_BodyTestEnd-g_BodyTest,   (char*)"?",             NULL);
  _bin_print((char*)"BodyBoot", (char*)"sysboot.bin",       g_BodyBootEnd-g_BodyBoot,   bodyBootHwId_Vers(str), bodyBootHwId_Debug(str));
  _bin_print((char*)"Body",     (char*)"syscon.bin",        g_BodyEnd-g_Body,           NULL,                   NULL);
  sprintf(str,"0x%08X",g_canary);
  _bin_print((char*)"Canary",   str,                        0,                          NULL,                   NULL);
}

