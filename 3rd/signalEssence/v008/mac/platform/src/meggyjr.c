/** meggyjr.c
 * 
 * a little program for sending RGB bitmaps to 
 * a meggy jr.
 *
 * This is handled in a threaded/thread-safe way, so that the call
 * returns are immediate and do not wait on IO being completed.
 * 
 * The meggy must be programmed with the serial IO demo program.
 *
 * MeggyJr serial format is simple:
 *   host              meggy                   description
 *   'h'               responds with 0xff      hello
 *   'd' x y color     no response             draw a pixel at x y in
 *                                             color.  colors are:
 *                                             0=dark, 
 *                                             1=red,
 *                                             2=orange
 *                                             3=yellow,
 *                                             4=green,
 *                                             5=blue,
 *                                             6=vilot,
 *                                             7=white,
 *                                             8=dim red,
 *                                             9=dim orange
 *                                             10=dim yellow,
 *                                             11=dim green,
 *                                             12=dim aqua
 *                                             13=dim blue,
 *                                             14=dim vilot,
 *                                             15=extra bright
 *
 *   'a' 'A' aux-leds     no response          set the top row LEDS to
 *                                             <aux-leds>
 *  
 */
#include <string.h>
#include <termios.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <pthread.h>
#include "meggyjr.h"

struct  MeggyStatus {
    int baud ;
    int handle;
    MeggySlate_t slate;
    MeggySlate_t newSlate;
    int newSlateAvailable;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t  cond;
} MeggyStatus = {
    .baud = B9600,
    .handle = 0,
    .newSlateAvailable = 0,
    .mutex = PTHREAD_MUTEX_INITIALIZER,
    .cond  = PTHREAD_COND_INITIALIZER
};


void MeggySendSlate(MeggySlate_t *slate);

/**
 * look for meggy.
 * return 0 on success, 1 on failure
 */
int MeggySayHi(struct MeggyStatus *meggy) {
    char msg[] = "h";
    unsigned char rcvd[10];
    int count = write(meggy->handle, msg, 1);
    memset(rcvd, 0, 10);
    if(count != 1) {
        return 1;
    }
    count = read(meggy->handle, rcvd, 1);
    if (count > 0 && rcvd[0] == 0xFF) {
        // good.
        return 0;
    } else {
        printf("Couldn't find meggy!\n");
        return 1;
    }
}

int MeggySetSlate(MeggySlate_t *slate) {
    if (!MeggyStatus.newSlateAvailable) {
        // the last one was sent, so we can set a new one.
      memcpy(&MeggyStatus.newSlate, slate, sizeof(MeggyStatus.slate));
        MeggyStatus.newSlateAvailable = 1;
	pthread_mutex_lock(&MeggyStatus.mutex);
	pthread_cond_signal(&MeggyStatus.cond);
	pthread_mutex_unlock(&MeggyStatus.mutex);
        return 1;
    } else {
        return 0;
    }
    
}
void *meggyRun(void *meggyvp) {
    struct MeggyStatus *meggy = (struct MeggyStatus *)meggyvp;
    while (1) {
        // forever, wait for a new slate, and send it.
        // not really worrying about locks and the like -- we only
        // consume, and meggySetSlate only produces.
      pthread_mutex_lock(&meggy->mutex);
      pthread_cond_wait(&meggy->cond, &meggy->mutex);
      pthread_mutex_unlock(&meggy->mutex);
      if (meggy->newSlateAvailable) {
	MeggySendSlate(&meggy->newSlate);
	meggy->newSlateAvailable = 0;
      }
    }
}
void startMeggyThread(struct MeggyStatus *meggy) {
    pthread_create(&meggy->thread, NULL, meggyRun, meggy);
}
/** MeggySerialInit
 * @param port the '/dev/ttyUSB0' port.
 * @param baud a baud rate -- like B9600 B19200, etc.  Use the Bxxx
 *             preprocessor macro and not a regular number .
 *
 * @returns 0 on success, nonzero on error.
 */
int MeggySerialInit(struct MeggyStatus *meggy, const char *port, int baud) {
    struct termios term;
    meggy->baud = baud;
    meggy->handle = open(port, O_RDWR | O_NOCTTY );
    if  (meggy->handle <= 0) {
        printf ("MeggySerialInit Couldn't open port %s\n", port);
        return 1;
    }
    if (tcgetattr(meggy->handle, &term) != 0) {
        printf("MeggySerialInit: couldn't tcgetattr\n");
        close(meggy->handle);
        return 1;
    }
    term.c_iflag &= ~(IGNBRK|BRKINT|PARMRK|INPCK|ISTRIP|INLCR|IGNCR|ICRNL|IXON|IXOFF);
    term.c_iflag |= IGNPAR;
    term.c_cflag &= ~(CSIZE|PARENB|PARODD|HUPCL|CRTSCTS);
    term.c_cflag |= CREAD|CS8|CSTOPB|CLOCAL;
    term.c_lflag &= ~(ISIG|ICANON|IEXTEN|ECHO);
    term.c_lflag |= NOFLSH;
    cfsetospeed(&term, baud);
    cfsetispeed(&term, baud);
    if (tcsetattr(meggy->handle, TCSANOW, &term) != 0) {
        printf("MeggySerialInit couldn't set serial port parameters\n");
        close(meggy->handle);
        return 1;
    }
    return MeggySayHi(meggy);
}


void ClearSlate(MeggySlate_t *slate) {
    int row, col;
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            slate->s[row][col] = 0;
        }
    }
}


void MeggySendPixel(int row, int col, int color) {
    unsigned char msg[5];
    int count;
    msg[0] = 'd';
    msg[1] = row;
    msg[2] = col;
    msg[3] = color;
    msg[4] = 'D';
    count = write(MeggyStatus.handle, msg, 5);
    if (count != 5) {
	fprintf(stderr, "MeggySendSlate didn't send 5 bytes\n");
    }
}
void MeggySendClearSlate(void) {
    int row, col;
    for (row = 0; row < 8; row++) {
	for (col = 0;col < 8; col++) {
	    MeggySendPixel(row, col, MeggyDark);
	}
    }
}
void MeggySendSlate(MeggySlate_t *slate) {
    MeggySlate_t diffs;
    int row, col;
    // First, calculate change bytes.
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            diffs.s[row][col] = (slate->s[row][col] != MeggyStatus.slate.s[row][col]);
        }
    }
    memcpy(MeggyStatus.slate.s, slate->s, sizeof(MeggyStatus.slate.s));
    // Then send the changed bytes;
    for (row = 0; row < 8; row++) {
        for (col = 0; col < 8; col++) {
            if (diffs.s[row][col]) 
		MeggySendPixel(row, col, MeggyStatus.slate.s[row][col]);
        }
    }
}

int MeggyJrInit(const char *port) {
    int err = MeggySerialInit(&MeggyStatus, port, B115200);
    if (err) {
        printf("Couldn't open meggy Jr serial port.\n");
        return err;
    }
    ClearSlate(&MeggyStatus.slate);
    
    // Start the watching thread.
    startMeggyThread(&MeggyStatus);
    MeggySendClearSlate();
    return 0;
}

#if 0
int main(int argc, char *argv) {
    MeggyJrInit("/dev/ttyUSB0");
    MeggySlate_t slate;
    ClearSlate(&slate);
    slate.s[2][2] = 3;
    MeggySendSlate(&slate);
    slate.s[2][4] = 5;
    slate.s[1][2] = 8;
    MeggySendSlate(&slate);
}
#endif
