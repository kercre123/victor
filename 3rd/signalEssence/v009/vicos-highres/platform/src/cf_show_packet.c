//============================================================================
// 635 Linux Demonstration Code.
// show_packet,c: Ultra-simple 635 command-line communications example
// Copyright 2005, Crystalfontz America, Inc. Written by Brent A. Crosby
// www.crystalfontz.com, brent@crystalfontz.com
//============================================================================
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "cf_typedefs.h"
#include "cf_packet.h"
#include "cf_show_packet.h"
//============================================================================
const char
 *command_names[36]=
   {" 0 = Ping",
    " 1 = Read Version",
    " 2 = Write Flash",
    " 3 = Read Flash",
    " 4 = Store Boot State",
    " 5 = Reboot",
    " 6 = Clear LCD",
    " 7 = LCD Line 1",	//not a valid command for the 635 module
    " 8 = LCD Line 2",  //not a valid command for the 635 module
    " 9 = LCD CGRAM",
    "10 = Read LCD Memory",
    "11 = Place Cursor",
    "12 = Set Cursor Style",
    "13 = Contrast",
    "14 = Backlight",
    "15 = Read Fans",
    "16 = Set Fan Rpt.",
    "17 = Set Fan Power",
    "18 = Read DOW ID",
    "19 = Set Temp. Rpt",
    "20 = DOW Transaction",
    "21 = Set Live Display",
    "22 = Direct LCD Command",
    "23 = Set Key Event Reporting",
    "24 = Read Keypad, Polled Mode",
    "25 = Set Fan Fail-Safe",
    "26 = Set Fan RPM Glitch Filter",
    "27 = Read Fan Pwr & Fail-Safe",
    "28 = Set ATX switch functionality",
    "29 = Watchdog Host Reset",
    "30 = Rd Rpt",
    "31 = Send Data to LCD (row,col)",
    "32 = Key Legends",
    "33 = Set Baud Rate",
    "34 = Set/Configure GPIO",
    "35 = Read GPIO & Configuration"};
const char
  *error_names[36]=
   {"Error: Ping",
    "Error: Version",
    "Error: Write Flash",
    "Error: Read Flash",
    "Error: Store Boot State",
    "Error: Reboot",
    "Error: Clear LCD",
    "Error: Set LCD 1",
    "Error: Set LCD 2",
    "Error: Set LCD CGRAM",
    "Error: Read LCD Memory",
    "Error: Place LCD Cursor",
    "Error: LCD Cursor Style",
    "Error: Contrast",
    "Error: Backlight",
    "Error: Query Fan",
    "Error: Set Fan Rept.",
    "Error: Set Fan Power",
    "Error: Read DOW ID",
    "Error: Set Temp. Rept.",
    "Error: DOW Transaction",
    "Error: Setup Live Disp",
    "Error: Direct LCD Command",
    "Error: Set Key Event Reporting",
    "Error: Read Keypad, Polled Mode",
    "Error: Set Fan Fail-Safe",
    "Error: Set Fan RPM Glitch Filter",
    "Error: Read Fan Pwr & Fail-Safe",
    "Error: Set ATX switch functionality",
    "Error: Watchdog Host Reset",
    "Error: Read  Reporting/ATX/Watchdog",
    "Error: Send Data to LCD (row,col)",
    "Error: Key Legends",
    "Error: Set Baud Rate",
    "Error: Set/Configure GPIO",
    "Error: Read GPIO & Configuration"};
const char
  *key_names[21]=
   {"KEY_NONE",
    "KEY_UP_PRESS",
    "KEY_DOWN_PRESS",
    "KEY_LEFT_PRESS",
    "KEY_RIGHT_PRESS",
    "KEY_ENTER_PRESS",
    "KEY_EXIT_PRESS",
    "KEY_UP_RELEASE",
    "KEY_DOWN_RELEASE",
    "KEY_LEFT_RELEASE",
    "KEY_RIGHT_RELEASE",
    "KEY_ENTER_RELEASE",
    "KEY_EXIT_RELEASE",
    "KEY_UL_PRESS",
    "KEY_UR_PRESS",
    "KEY_LL_PRESS",
    "KEY_LR_PRESS",
    "KEY_UL_RELEASE",
    "KEY_UR_RELEASE",
    "KEY_LL_RELEASE",
    "KEY_LR_RELEASE"};
//----------------------------------------------------------------------------
void cf_ShowReceivedPacket(void)
  {
  int
    i;
  //Terminate the incoming data so C handles it well in case it is a string.
  incoming_command.data[incoming_command.data_length]=0;
  char
    expanded[400];
  expanded[0]=0;
  for(i=0;i<incoming_command.data_length;i++)
    {
    if(isprint(incoming_command.data[i]))
      {
      int
        current_length=strlen(expanded);
      expanded[current_length]=incoming_command.data[i];
      expanded[current_length+1]=0;
      }
    else
      {
      char
        bin_number[5];
      sprintf(bin_number,"\\%03d",incoming_command.data[i]);
      strcat(expanded,bin_number);
      }
    }

  //key
  if(incoming_command.command==0x80)
    printf("C=%d(key:%s),L=%d,D=\"%s\",CRC=0x%04X\n",
      incoming_command.command,incoming_command.data[0]<=20?
        key_names[incoming_command.data[0]]:
          "HUH?",
      incoming_command.data_length,
      expanded,
      incoming_command.CRC.as_word);
  else
	//any other packet types
    printf("C=%d(%s),L=%d,D=\"%s\",CRC=0x%04X\n",
      incoming_command.command,
      ((incoming_command.command&0xC0)==0xC0)?
        error_names[0x3F&incoming_command.command]:
          command_names[0x3F&incoming_command.command],
      incoming_command.data_length,
      expanded,
      incoming_command.CRC.as_word);
}
//============================================================================
