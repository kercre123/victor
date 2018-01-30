/**
 ****************************************************************************************
 *
 * @file console.c
 *
 * @brief Basic console user interface form proximity monitor host application.
 *
 * Copyright (C) 2012. Dialog Semiconductor Ltd, unpublished work. This computer 
 * program includes Confidential, Proprietary Information and is a Trade Secret of 
 * Dialog Semiconductor Ltd.  All use, disclosure, and/or reproduction is prohibited 
 * unless authorized in writing. All Rights Reserved.
 *
 * <bluetooth.support@diasemi.com> and contributors.
 *
 ****************************************************************************************
 */

#define __BOOL_DEFINED

#include <conio.h>
#include <ctype.h>
#include <stdio.h>
#include "rwble_config.h"
#include "queue.h"
#include "console.h"
#include "app.h"
#include "proxm.h"

bool console_mode = false;					//Display of selected connection info 
bool giv_num_state = false;					//Print the "Give Device number!!" message			
bool conn_num_flag = false;					//Print the "Max connections Reached" message
bool console_main_pass_state = false;		//Zero only if no connection has been established
bool keyflag = false;						//For the up-down arrows 

/*
 ****************************************************************************************
 * @brief Sends Discover devices message to application 's main thread.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendScan(void)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = CONSOLE_DEV_DISC_CMD;
    
    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
}

/*
 ****************************************************************************************
 * @brief Sends Connect to device message to application 's main thread.
 *
 *  @param[in] indx Device's index in discovered devices list.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendConnnect(int indx)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = CONSOLE_CONNECT_CMD;
    pmsg->idx = indx;

    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
}

/*
 ****************************************************************************************
 * @brief Sends Disconnect request message to application 's main thread.
 *
 *  @param[in] id Selected device id.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendDisconnnect(int id)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = CONSOLE_DISCONNECT_CMD;
	pmsg->val = id;
    
    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
	
}

/*
 ****************************************************************************************
 * @brief Sends Read request message to application 's main thread.
 *
 *  @param[in] type  Attribute type to read.
 *  @param[in] idx  Selected device id.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleRead(unsigned char type , unsigned char idx)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = type;
	pmsg->idx = idx;

    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
}

/*
 ****************************************************************************************
 * @brief Sends write request message to application 's main thread.
 *
 *  @param[in] type  Attribute type to write.
 *  @param[in] val   Attribute value.
 *  @param[in] idx   Selected device id.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleWrite(unsigned char type, unsigned char val, unsigned char idx)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = type;
    pmsg->val = val;
	pmsg->idx = idx;

    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
}

/*
 ****************************************************************************************
 * @brief Sends a message to application 's main thread to exit.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleSendExit(void)
{
    console_msg *pmsg = (console_msg *) malloc(sizeof(console_msg));

    pmsg->type = CONSOLE_EXIT_CMD;
    
    WaitForSingleObject(ConsoleQueueSem, INFINITE);
    EnQueue(&ConsoleQueue, pmsg);
    ReleaseMutex(ConsoleQueueSem);
}

/*
 ****************************************************************************************
 * @brief Handles key-press events and sends corresponding message to application's main thread.
 *
 *  @param[in] key  input from keyboard
 *
 * @return void.
 ****************************************************************************************
*/
void HandleKeyEvent(int Key)
{
	int sel_dev;
    switch(Key)
    {
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
			if(!console_main_pass_state && app_env.devices[Key - 0x31].free==false){											//Runs only when there are no-connections
            printf ("\t\t\t\tTrying to connect to %c... Please wait \n\n", Key); // NG tmp  
            ConsoleSendConnnect(Key - 0x31);
			app_cancel_gap();
			Sleep(100);
			app_inq();																//refresh the scan list
			}
			else if (app_env.proxr_device[(Key - 0x31)].isactive == true)			//modifies the cur_dev indicator
			{
				app_env.cur_dev=(Key - 0x31);
			}
            break;
        case 'S':
            ConsoleSendScan();
            break;
        case 'A':
			ConsoleRead(CONSOLE_RD_LLV_CMD,app_env.cur_dev);
			Sleep(100);
            break;
        case 'B':
            ConsoleRead(CONSOLE_RD_TXP_CMD,app_env.cur_dev);
			Sleep(100);
            break;
        case 'C':
            ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_HIGH,app_env.cur_dev);
            break;
        case 'D':
            ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_MILD,app_env.cur_dev);
            break;
        case 'E':
            ConsoleWrite(CONSOLE_WR_IAS_CMD, PROXM_ALERT_NONE,app_env.cur_dev);
            break;
        case 'F':
            ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_NONE,app_env.cur_dev);
			Sleep(100);	
			ConsoleRead(CONSOLE_RD_LLV_CMD,app_env.cur_dev);
            break;
        case 'G':
            ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_MILD,app_env.cur_dev);
			Sleep(100);
			ConsoleRead(CONSOLE_RD_LLV_CMD,app_env.cur_dev);
            break;
        case 'H':
            if(keyflag && app_env.cur_dev>0) {														// up-down arrow come with a special character command	
				sel_dev=app_env.cur_dev;															// after the special character is detected it sets the key-flag
				if (app_env.proxr_device[(sel_dev-1)%MAX_CONN_NUMBER].isactive == true){			// and this is the "up arrow"
				sel_dev=(sel_dev-1)%MAX_CONN_NUMBER;
				}else{
				while(app_env.proxr_device[(sel_dev-1)%MAX_CONN_NUMBER].isactive == false){
					sel_dev=(sel_dev-1)%MAX_CONN_NUMBER;
					if(app_env.proxr_device[(sel_dev-1)%MAX_CONN_NUMBER].isactive == true){
						sel_dev=(sel_dev-1)%MAX_CONN_NUMBER;
						break;
					}
				}}
				if (sel_dev>=0){
				app_env.cur_dev=sel_dev;
				}
				keyflag=false;}
			else{
			ConsoleWrite(CONSOLE_WR_LLV_CMD, PROXM_ALERT_HIGH,app_env.cur_dev);					
			Sleep(100);
			ConsoleRead(CONSOLE_RD_LLV_CMD,app_env.cur_dev);
			}
            break;
        case 'Q':
			ConsoleConnected(1);
            if (!console_mode)
                console_mode = true;
            else
				console_mode = false;
			break;
		case 'P':
			if(keyflag && app_env.cur_dev<MAX_CONN_NUMBER) {												// up-down arrow come with a special character command
				sel_dev=app_env.cur_dev;																	// after the special character is detected it sets the key-flag
				if (app_env.proxr_device[(sel_dev+1)%MAX_CONN_NUMBER].isactive == true){					// and this is the "down arrow"
				sel_dev=(sel_dev+1)%MAX_CONN_NUMBER;
				}else{
				while(app_env.proxr_device[(sel_dev+1)%MAX_CONN_NUMBER].isactive == false){
					sel_dev=(sel_dev+1)%MAX_CONN_NUMBER;
					if(app_env.proxr_device[(sel_dev+1)%MAX_CONN_NUMBER].isactive == true){
						sel_dev=(sel_dev+1)%MAX_CONN_NUMBER;
						break;
					}
				}}
				if (sel_dev<=MAX_CONN_NUMBER-1){
				app_env.cur_dev=sel_dev;
				}
				keyflag=false;}

																										
			else if(rtrn_fst_avail()<MAX_CONN_NUMBER){													//Connection command 
				app_cancel_gap();																		
				giv_num_state=true; 
				Sleep(100);
				ConsoleSendConnnect((int) ConsoleReadPort()-0x01);
				}
			else{
				conn_num_flag=true;
			}
			giv_num_state=false;
            break;
        case 'I':
            ConsoleSendDisconnnect(app_env.cur_dev);
            break;
        case 0x1B:
            ConsoleSendExit();
            break;
		case 0xE0:
			keyflag=true;
			break;
        default:
            break;
    }
}

/*
 ****************************************************************************************
 * @brief Demo application Console Menu header
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleTitle(void)
{
    system("cls"); //clear screen
    printf ("\t  ####################################################\n");
    printf ("\t  #    DA1458x Proximity Monitor demo application    #\n");
    printf ("\t  ####################################################\n\n");
}

/*
 ****************************************************************************************
 * @brief Prints Console menu in Scan state.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleScan(void)
{
	if(!console_main_pass_state){
    ConsoleTitle();

    printf ("\t\t\t\tScanning... Please wait \n\n");    

    ConsoleIdle();
	}
}

/*
 ****************************************************************************************
 * @brief Prints Console menu in Idle state.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleIdle(void)
{
	if(!console_main_pass_state){
    ConsoleTitle();

    ConsolePrintScanList();

    printf ("Select number of device to connect or 'S' to Rescan:\n");
}
}

/*
 ****************************************************************************************
 * @brief Prints List of discovered devices.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsolePrintScanList(void)
{
    int index, i;

	if(!console_main_pass_state){
    console_mode = false;
	}

	printf ("#  No. \tbd_addr \t\tName \t\tRssi\t\t\t#\n");

    for (index=0; index<MAX_SCAN_DEVICES; index++)
    {
        if (app_env.devices[index].free == false)
        {
			printf("#  %d\t", index+1);
            for (i=0; i<BD_ADDR_LEN-1; i++)
                printf ("%02x:", app_env.devices[index].adv_addr.addr[BD_ADDR_LEN - 1 - i]);
            printf ("%02x", app_env.devices[index].adv_addr.addr[0]);
			if(app_env.devices[index].data=='\0'){
				printf ("\t\t\t%d dB\t\t\t#\n", app_env.devices[index].rssi);
			}
			else{
				if(app_env.devices[index].data[1]==0){ 
					printf ("\t%s\t\t\t%d dB\t\t\t#\n", app_env.devices[index].data, app_env.devices[index].rssi);
				}
				else if(app_env.devices[index].data[5]==0){ 
					printf ("\t%s\t\t%d dB\t\t\t#\n", app_env.devices[index].data, app_env.devices[index].rssi);
				}
				else {
					printf ("\t%s\t%d dB\t\t\t#\n", app_env.devices[index].data, app_env.devices[index].rssi);	
				}
			}
        }
    }
}

/*
 ****************************************************************************************
 * @brief Prints Console Menu in connected state.
 *
 * @param[in] full   If true, prints peers attributes values.
 *
 * @return void.
 ****************************************************************************************
*/
void ConsoleConnected(int full)
{
    int i;
    ConsoleTitle();
	printf ("\n#########################################################################\n");
	printf ("############################   Scan List   ##############################\n");
	printf ("#########################################################################\n");
	printf ("#                                                                       #\n");
	console_main_pass_state=true;																					
	if (conn_num_flag) print_err_con_num();															//checks for full connection slots
	else ConsolePrintScanList();
	printf ("#                                                                       #\n");
	printf ("#########################################################################");
	ConsoleQues();
    if (!console_mode)
    {
		printf ("#############################  Options  #################################\n");
		printf ("#########################################################################\n");
		printf ("#                                                                       #\n");
		printf ("#  'A' - Read Link Loss Alert Level                                     #\n"); // NG tmp
		printf ("#  'B' - Read Tx Power Level                                            #\n");
		printf ("#  'C' - Start High Level Immediate Alert                               #\n"); // NG tmp
		printf ("#  'D' - Start Mild Level Immediate Alert                               #\n"); // NG tmp
		printf ("#  'E' - Stop Immediate Alert                                           #\n"); // NG tmp
		printf ("#  'F' - Set Link Loss Alert Level to None                              #\n");
		printf ("#  'G' - Set Link Loss Alert Level to Mild                              #\n");
		printf ("#  'H' - Set Link Loss Alert Level to High                              #\n");
		printf ("#  'P' - Connect to device                                              #\n");
		printf ("#  'I' - Disconnect from device                                         #\n");
		printf ("#  'S' - Rescan Advertising devices                                     #\n");
    }
    else
    {
			printf ("#                                                                       #\n");
			printf ("#  Device Information                                                   #\n");
			printf ("#  ------------------                                                   #\n");
			printf ("#                                                                       #\n");
			printf ("#  Manufacturer: %s  \t\t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MANUFACTURER_NAME_CHAR].val);
			printf ("#  Nodel Number: %s  \t\t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_MODEL_NB_STR_CHAR].val);
            printf ("#  Serial Number: %s \t\t\t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SERIAL_NB_STR_CHAR].val);
			printf ("#  Hardware Revision: %s \t\t\t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_HARD_REV_STR_CHAR].val);
			printf ("#  Firmware Revision: %s \t\t\t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_FIRM_REV_STR_CHAR].val);
			printf ("#  Software Revision: %s \t\t\t\t\t#\n", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SW_REV_STR_CHAR].val);
            printf ("#  System ID: %02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].val[0]);
            for (i=1; i<app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].len; i++)
                printf (":%02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_SYSTEM_ID_CHAR].val[i]);
			printf("\t\t\t\t\t#\n");
            printf ("#  IEEE Certification: %02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].val[0]);
            for (i=1; i<app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].len; i++)
                printf (":%02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_IEEE_CHAR].val[i]);
			printf("\t\t\t\t\t\t#\n");
            printf ("#  PnP ID: %02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].val[0]);
            for (i=1; i<app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].len; i++)
                printf (":%02x", app_env.proxr_device[app_env.cur_dev].dis.chars[DISC_PNP_ID_CHAR].val[i]);
			printf ("\t\t\t\t\t\t\t\t#\n");
		printf ("#                                                                       #\n");
		printf ("#########################################################################\n");
		printf ("#############################  Options  #################################\n");
		printf ("#########################################################################\n");
    }
	printf ("#                                                                       #\n");
	printf ("#  'Q' - Display/Hide Device Information                                #\n");
	printf ("#  'Esc' - Exit                                                         #\n");
	printf ("#                                                                       #\n");
	printf ("#########################################################################\n");
}

/*
 ****************************************************************************************
 * @brief Console Thread main loop.
 *
 * @return If the message was consumed or not.
 ****************************************************************************************
*/
VOID ConsoleProc(PVOID unused)
{
   int Key = 0xFF;

   while(StopConsoleTask == FALSE)
   {
        Key = _getch();  // Get keypress 
        Key = toupper(Key);
        HandleKeyEvent(Key);
   }

   StopConsoleTask = FALSE;   // To indicate that the task has stopped
   ExitThread(0);
}


/*
 ****************************************************************************************
 * @brief Prints connected devices list
 * 
 * @return void
 ****************************************************************************************
*/
void ConsoleQues(void)
{
	int cnt,i;
	char stmp[16];
	
	printf ("\n#########################   Connected Devices   #########################\n");
	printf ("#########################################################################\n");
	printf ("#                                                                       #\n");
	printf ("#  No. Model No.\tBDA\t\t   Bonded  RSSI   LLA  TX_PL\t#\n");
	for (cnt=0;cnt<MAX_CONN_NUMBER;cnt++){
		if (cnt==app_env.cur_dev){
			printf ("#  * %d",cnt+1);//Active Counter
		}
		else{
			printf ("#    %d",cnt+1);//Counter
		}									
		if(app_env.proxr_device[cnt].isactive == true){
			if (app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val[0]=='\0'){
				printf (" \t\t\t");
			} 
			else if (app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val[10]!='\0'){
				strncpy(stmp,app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val,15);
				stmp[15]='\0';
				printf (" %s\t",stmp);			//Model Number
			}else{
				printf (" %s\t\t",app_env.proxr_device[cnt].dis.chars[DISC_MODEL_NB_STR_CHAR].val);
			}
			for (i=0; i<BD_ADDR_LEN-1; i++)																			//BDA
				printf ("%02x:", app_env.proxr_device[cnt].device.adv_addr.addr[BD_ADDR_LEN - 1 - i]);
			printf ("%02x  ", app_env.proxr_device[cnt].device.adv_addr.addr[0]);					
			if (app_env.proxr_device[cnt].bonded)														//BONDED
				printf (" YES\t  ");
			else
				printf (" NO\t  ");
			if (app_env.proxr_device[cnt].avg_rssi != -127)												//RSSI
				printf ("%02d dB", app_env.proxr_device[cnt].avg_rssi);
			else
				printf ("\t");
			if (app_env.proxr_device[cnt].alert)
				printf ("\a PATH LOSS ALERT");
			else {
				if (app_env.proxr_device[cnt].llv != 0xFF)									//Link Loss Alert Level
					printf ("   %02d\t", app_env.proxr_device[cnt].llv);
				else
					printf ("    \t");
				if (app_env.proxr_device[app_env.cur_dev].txp != -127)									//Tx Power Level
					printf (" %02d\t", app_env.proxr_device[cnt].txp);
				else
					printf ("    \t");
			}
			printf ("#\n");
		}
		else{
			printf ("                --   Empty Slot --                                #\n");
		}
	}
	printf ("#                                                                       #\n");
	printf ("#########################################################################\n");
	if (giv_num_state){
		printf ("#                                                                       #\n");
		printf ("#---------------------------  Select Device  ---------------------------#\n"); 
		printf ("#                                                                       #\n");
	}

}


/*
 ****************************************************************************************
 * @brief prints message "maximum connections reached"
 *
 * @return void.
 ****************************************************************************************
*/
void print_err_con_num(void){

	printf ("#########################################################################\n");
	printf ("##                    max no of connections reached                    ##\n");
	printf ("#########################################################################\n");


}