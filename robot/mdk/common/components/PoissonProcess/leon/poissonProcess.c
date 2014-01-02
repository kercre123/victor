#include "poissonProcess.h"
#include "DrvTimer.h"
#include "swcRandom.h"
#include "swcLeonMath.h"
#include "math.h"

#define POISSON_IRQ_PRIORITY (3)

static tyPoissonProcess *processes[NUM_TIMERS + 1];
static int process_nyet_init;

void poissonInit(void) {
    int i;
    for (i=0;i<NUM_TIMERS+1;i++) processes[i] = NULL;
}

static u32 poissonNextEvent(tyPoissonProcess *process) {
    double dividend = swcLongLongToDouble(swcRandGetRandValue_r(&(process->seed)));
    double divisor = (double)(RAND_MAX) + 1;
    double value_0_1ex; // value between [0,1)
    value_0_1ex = dividend / divisor;
    float value_0ex_1; // value between (0, 1]
    value_0ex_1 = 1 - value_0_1ex;
    float time = - (process->averageMicroSecondsPerEvent * logf(value_0ex_1));
    if (time > ((u32)(-1))) time = (u32)(-1);
    return (u32) time;
}

static u32 poissonCallback(u32 timerNumber, u32 param2);

void poissonProcess(tyPoissonProcess *process) {
    u32 time = poissonNextEvent(process);
    if (time == 0) time = 1; // highly unlikely, but still
    process_nyet_init = 0;
    int tindex = DrvTimerCallAfterMicro(time, poissonCallback, 1, POISSON_IRQ_PRIORITY);
    processes[tindex] = process;
    asm volatile ("" : : : "memory"); //memory barrier, to force this order of memory access.
    if (process_nyet_init) {
        poissonCallback(tindex, 0);
    }
}

static u32 poissonCallback(u32 timerNumber, u32 param2) {
    tyPoissonProcess *p = processes[timerNumber];
    processes[timerNumber] = NULL;
    if (p==NULL) {
        process_nyet_init = 1;
        return 0;
    }
    p->callback(p->private);
    poissonProcess(p);

    return 0;
}
