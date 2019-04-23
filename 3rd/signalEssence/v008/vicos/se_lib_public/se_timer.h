#ifdef __cplusplus
extern "C" {
#endif

#ifndef SE_TIMER_H
#define SE_TIMER_H
	
/***************************************************************************
   (C) Copyright 2015 SignalEssence; All Rights Reserved

    Module Name: se_timer.h

    Author: Caleb Crome

    Description: simple timer functions to probe time it takes for 
      parts of the algorithm to run.
    

    Machine/Compiler:
    (ANSI C)
**************************************************************************/
void se_timer_start(const char *name, int indent);
void se_timer_stop(const char *name);
void se_timer_reset(const char *name);
void se_timer_show_all_timers(void);
double se_timer_get_cumulative(const char *name);
#endif
#ifdef __cplusplus
}
#endif
