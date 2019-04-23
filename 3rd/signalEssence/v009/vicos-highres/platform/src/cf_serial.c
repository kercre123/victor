//============================================================================
// 635 Linux Demonstration Code.
// serial,c: Ultra-simple 635 command-line communications example
// Copyright 2005, Crystalfontz America, Inc. Written by Brent A. Crosby
// www.crystalfontz.com, brent@crystalfontz.com
//============================================================================
#include  <stdlib.h>
#include  <termios.h>
#include  <unistd.h>
#include  <fcntl.h>
#include  <errno.h>
#include  <ctype.h>
#include  <stdio.h>
#include  "cf_typedefs.h"
#include  "cf_serial.h"
#include "se_types.h"

#define FALSE   0
#define TRUE    1

// com port handle
static int cf_handle;

// data buffering
#define RECEIVEBUFFERSIZE 4096
static ubyte SerialReceiveBuffer[RECEIVEBUFFERSIZE];
static dword ReceiveBufferHead;
static dword ReceiveBufferTail;
static dword ReceiveBufferTailPeek;

//------------------------------------------------------------------------------
int cf_Serial_Init(char *devname, int baud_rate)
  {
  int
    brate;
  struct
    termios term;

  //open device
  cf_handle = open(devname, O_RDWR | O_NOCTTY | O_NONBLOCK);

  if(cf_handle <= 0)
    {
      	printf("Serial_Init:: Open() failed\n");
	return(1);
    }

  //get baud rate constant from numeric value
  switch (baud_rate)
  {
    case 19200:
      brate=B19200;
      break;
    case 115200:
      brate=B115200;
      break;
    default:
      printf("Serial_Init:: Invalid baud rate: %d (must be 19200 or 115200)\n", baud_rate);
      return(2);
  }
  //get device struct
  if(tcgetattr(cf_handle, &term) != 0)
    {
    printf("Serial_Init:: tcgetattr() failed\n");
    return(3);
    }

  //input modes
  term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL
                  |IXON|IXOFF);
  term.c_iflag |= IGNPAR;

  //output modes
  term.c_oflag &= ~(OPOST|ONLCR|OCRNL|ONOCR|ONLRET|OFILL
                  |OFDEL|NLDLY|CRDLY|TABDLY|BSDLY|VTDLY|FFDLY);

  //control modes
  term.c_cflag &= ~(CSIZE|PARENB|PARODD|HUPCL|CRTSCTS);
  term.c_cflag |= CREAD|CS8|CSTOPB|CLOCAL;

  //local modes
  term.c_lflag &= ~(ISIG|ICANON|IEXTEN|ECHO);
  term.c_lflag |= NOFLSH;

  //set baud rate
  cfsetospeed(&term, brate);
  cfsetispeed(&term, brate);

  //set new device settings
  if(tcsetattr(cf_handle, TCSANOW, &term)  != 0)
    {
    printf("Serial_Init:: tcsetattr() failed\n");
    return(4);
    }

  ReceiveBufferHead=ReceiveBufferTail=0;

//  printf("Serial_Init:: success\n");
  return(0);
  }
//------------------------------------------------------------------------------
void cf_Uninit_Serial()
  {
  close(cf_handle);
  cf_handle=0;
  }
//------------------------------------------------------------------------------
void cf_SendByte(unsigned char datum)
  {
  dword
    bytes_written;
  bytes_written=0;
  if(cf_handle)
    {
    if((bytes_written=write(cf_handle, &datum, 1)) != 1)
      printf("SendByte(): system call \"write()\" return error.\n  Asked for %d bytes to be written, but %d bytes reported as written.\n",
             1,(int)bytes_written);
    }
  else
    printf("SendByte(): \"handle\" is null\n");
  }
//------------------------------------------------------------------------------
void cf_SendData(unsigned char *data,int length)
{
    int bytes_written;
    bytes_written=0;
    
    if(cf_handle)
    {
	if((bytes_written=write(cf_handle, data, length)) != length)
	    printf("SendData(): system call \"write()\" return error.\n  Asked for %d bytes to be written, but %d bytes reported as written.\n",
		   length,(int)bytes_written);
    }
    else
	printf("SendData(): \"handle\" is null\n");
    
}

//------------------------------------------------------------------------------
void cf_SendString(char *data)
  {
  while(*data)
    {
    usleep(500);
    cf_SendByte(*data++);
    }
  }
//------------------------------------------------------------------------------
//Gets incoming data and puts it into SerialReceiveBuffer[].
void cf_Sync_Read_Buffer(void)
  {
  ubyte
    Incoming[4096];
  unsigned long
    BytesRead;

  //  COMSTAT status;
  dword
    i;

  if(!cf_handle)
    return;

  //get data
  BytesRead = read(cf_handle, &Incoming, 4096);
  if(0<BytesRead)
  {
  //Read the incoming ubyte, store it.
  for(i=0; i < BytesRead; i++)
    {
    SerialReceiveBuffer[ReceiveBufferHead] = Incoming[i];

    //Increment the pointer and wrap if needed.
    ReceiveBufferHead++;
    if (RECEIVEBUFFERSIZE <= ReceiveBufferHead)
      ReceiveBufferHead=0;
      }
    }
  }
/*---------------------------------------------------------------------------*/
dword cf_BytesAvail(void)
  {
  //LocalReceiveBufferHead and return_value are a signed variables.
  int
    LocalReceiveBufferHead;
  int
    return_value;

  //Get a register copy of the head pointer, since an interrupt that
  //modifies the head pointer could screw up the value. Convert it to
  //our signed local variable.
  LocalReceiveBufferHead = ReceiveBufferHead;
  if((return_value = (LocalReceiveBufferHead - (int)ReceiveBufferTail)) < 0)
    return_value+=RECEIVEBUFFERSIZE;

  return(return_value);
  }
/*---------------------------------------------------------------------------*/
ubyte cf_GetByte(void)
  {
  dword
    LocalReceiveBufferTail;
  dword
    LocalReceiveBufferHead;
  ubyte
    return_byte=0;

  //Get a register copy of the tail pointer for speed and size.
  LocalReceiveBufferTail=ReceiveBufferTail;

  //Get a register copy of the head pointer, since an interrupt that
  //modifies the head pointer could screw up the value.
  LocalReceiveBufferHead=ReceiveBufferHead;


  //See if there are any more bytes available.
  if(LocalReceiveBufferTail!=LocalReceiveBufferHead)
    {
    //There is at least one more ubyte.
    return_byte=SerialReceiveBuffer[LocalReceiveBufferTail];

    //Increment the pointer and wrap if needed.
    LocalReceiveBufferTail++;
    if(RECEIVEBUFFERSIZE<=LocalReceiveBufferTail)
      LocalReceiveBufferTail=0;

    //Update the real ReceiveBufferHead with our register copy. Make sure
    //the ISR does not get serviced during the transfer.
    ReceiveBufferTail=LocalReceiveBufferTail;
    }

  return(return_byte);
  }
/*---------------------------------------------------------------------------*/
dword cf_PeekBytesAvail(void)
  {
  //LocalReceiveBufferHead and return_value are a signed variables.
  int
    LocalReceiveBufferHead;
  int
    return_value;

  //Get a register copy of the head pointer, since an interrupt that
  //modifies the head pointer could screw up the value. Convert it to
  //our signed local variable.
  LocalReceiveBufferHead=ReceiveBufferHead;
  if((return_value=(LocalReceiveBufferHead-(int)ReceiveBufferTailPeek))<0)
    return_value+=RECEIVEBUFFERSIZE;
  return(return_value);
  }
/*---------------------------------------------------------------------------*/
void cf_Sync_Peek_Pointer(void)
  {
  ReceiveBufferTailPeek=ReceiveBufferTail;
  }
/*---------------------------------------------------------------------------*/
void cf_AcceptPeekedData(void)
  {
  ReceiveBufferTail=ReceiveBufferTailPeek;
  }
/*---------------------------------------------------------------------------*/
ubyte cf_PeekByte(void)
  {
  int
    LocalReceiveBufferTailPeek;
  int
    LocalReceiveBufferHead;
  ubyte return_byte=0;

  //Get a register copy of the tail pointer for speed and size.
  LocalReceiveBufferTailPeek=ReceiveBufferTailPeek;

  //Get a register copy of the head pointer, since an interrupt that
  //modifies the head pointer could screw up the value.
  LocalReceiveBufferHead=ReceiveBufferHead;

  //See if there are any more bytes available.
  if(LocalReceiveBufferTailPeek!=LocalReceiveBufferHead)
    {
    //There is at least one more ubyte.
    return_byte=SerialReceiveBuffer[LocalReceiveBufferTailPeek];

    //Increment the pointer and wrap if needed.
    LocalReceiveBufferTailPeek++;
    if(RECEIVEBUFFERSIZE<=LocalReceiveBufferTailPeek)
      LocalReceiveBufferTailPeek=0;

    //Update the real ReceiveBufferHead with our register copy. Make sure
    //the ISR does not get serviced during the transfer.
    ReceiveBufferTailPeek=LocalReceiveBufferTailPeek;
    }
  return(return_byte);
  }


/*---------------------------------------------------------------------------*/

void cf_SetScroll(int on) {
	if (on) {
		cf_SendByte(19);
	} else {
		cf_SendByte(20);
	}
}
void cf_SetWrap(int on) {
	if (on) {
		cf_SendByte(23);
	} else {
		cf_SendByte(24);
	}
}
void cf_SetPos(int row, int col) {
	cf_SendByte(17);
	cf_SendByte(col - 1);
	cf_SendByte(row - 1);
}
void cf_EraseLine(int row) {
	cf_SetPos(row, 1);
	//          12345678901234567890
	cf_SendString("                    ");
}
void cf_SendStringRC(int row, int col, char *str) {
	cf_SetPos(row, col);
	cf_SendString(str);
}

void cf_Bargraph(
	int row,
	int start_col,
	int end_col,
	int style,
	int percent)
{
	int npixels = (end_col-start_col+1) * 6;
	
	float pct = (float)percent/100.0;
	int len = (int)((float)npixels * pct) + 0.5;
	UNUSED(style);
	if (len > npixels) 
		len = npixels;
	if  (len < 0)
		len = 0;

	cf_SendByte(18);
	cf_SendByte(0);
	cf_SendByte(255);
	cf_SendByte(start_col-1);
	cf_SendByte(end_col-1);
	cf_SendByte(len);
	cf_SendByte(row-1);
}
	
void cf_ClearDisplay(void)
{
	cf_EraseLine(0);
	cf_EraseLine(1);
	cf_EraseLine(2);
	cf_EraseLine(3);
}		      

void cf_HideCursor(void) {
	cf_SendByte(4);
}
