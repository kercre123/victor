/******************************************************************************/
/* RETARGET.C: 'Retarget' layer for target-dependent low level functions      */
/******************************************************************************/
/* This file is part of the uVision/ARM development tools.                    */
/* Copyright (c) 2005 Keil Software. All rights reserved.                     */
/* This software may only be used under the terms of a valid, current,        */
/* end user licence from KEIL for a compatible version of KEIL software       */
/* development tools. Nothing else gives you the right to use this software.  */
/******************************************************************************/

#include <stdio.h>
#include <time.h>
#include <rt_misc.h>

#pragma import(__use_no_semihosting_swi)


//extern int  sendchar(int ch);  /* in Serial.c */
//extern int  getkey(void);      /* in Serial.c */
extern long timeval;           /* in Time.c   */


struct __FILE { int handle; /* Add whatever you need here */ };
FILE __stdout;
FILE __stdin;

int getkey(void) {
	return 0;
}

int sendchar(int ch) {
	return 0;
}

int fputc(int ch, FILE *f) {
  return (sendchar(ch));
}

int fgetc(FILE *f) {
  return (sendchar(getkey()));
}


int ferror(FILE *f) {
  /* Your implementation of ferror */
  return EOF;
}


void _ttywrch(int ch) {
  sendchar (ch);
}


void _sys_exit(int return_code) {
  while (1);    /* endless loop */
}
