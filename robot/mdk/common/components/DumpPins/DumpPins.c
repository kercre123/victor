#include "DumpPins.h"
#include "DrvTimer.h"
#include "stdio.h"
#include "DrvGpio.h"

static int *dPins;
static int dPinNo;
static int *dBuffer;
static int dBufferSize; // words
static int dBufferPointer;
static int timerNumber;

static inline void sample() {
    int val = 0;
    int i = 0;
    for (i=0;i<dPinNo;i++) {
        val = (val << 1) | DrvGpioGet(dPins[i]);
    }
    u64 time = DrvTimerGetSysTicks64();
    if ((dBufferPointer == 0) || (val != dBuffer[dBufferPointer - 1])) {
        dBuffer[dBufferPointer++] = time & 0xffffffff;
        dBuffer[dBufferPointer++] = (time >> 32) & 0xffffffff;
        dBuffer[dBufferPointer++] = val;
    }
}

static u32 timerCallback(u32 timerNumber, u32 param2) {
    sample();
    if (dBufferPointer + 3 >= dBufferSize) {
        printf("Pin dumping buffer is full!\n");
        StopDumpingPins();
    }
}


void StopDumpingPins() {
    DrvTimerDisable(timerNumber);
    printf("Please run \"save 0x%08x %d BLE saved.val\"\n", (u32)(dBuffer), dBufferPointer * 4);
}


void DumpPins(int *pins, int pinNo, int *buffer, int bufferSize, int periodMicroSeconds) {
    if (pinNo > 32)
        printf("Error, too many pins to dump!\n");
    dPins = pins;
    dPinNo = pinNo;
    dBuffer = buffer;
    dBufferSize = bufferSize;
    dBufferPointer = 0;

    dBuffer[dBufferPointer++] = pinNo;
    int i;
    for (i=0;i<pinNo;i++) {
        dBuffer[dBufferPointer++] = pins[i];
    }
    timerNumber = DrvTimerCallAfterMicro(periodMicroSeconds, timerCallback, 0, 5);
}



