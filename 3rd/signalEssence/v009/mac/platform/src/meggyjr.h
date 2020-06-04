#ifndef _MEGGYJR_H
#define _MEGGYJR_H

typedef struct {
    int s[8][8];
} MeggySlate_t;


#define MeggyDark        0
#define MeggyRed         1
#define MeggyOrange      2
#define MeggyYellow      3
#define MeggyGreen       4
#define MeggyBlue        5
#define MeggyViolet      6
#define MeggyWhite       7
#define MeggyDimRed      8
#define MeggyDimOrange   9
#define MeggyDimYellow   10
#define MeggyDimGreen    11
#define MeggyDimAqua     12
#define MeggyDimBlue     13
#define MeggyDimVilot    14
#define MeggyExtraBright 15

/** MeggyJrInit
 * @param port the /dev/ttyUSBxyz to connect to.
 * @returns 0 on success, nonzero on error.
 */
int MeggyJrInit(const char *port);

/**
 * Schedules a new slate to be sent to the meggy. 
 * returns 1 if it will be sent or 0 if it's still busy sending the
 * last one.  No errors will happen, but your slate may not get sent.
 * If you really need it to be sent, well, send it again.
 *
 * @param slate a pointer to a new slate to send.  The data is copied,
 *              so you don't need to keep it around after the call to
 *              MeggySetSlate.
 */
int MeggySetSlate(MeggySlate_t *slate);

void ClearSlate(MeggySlate_t *slate);

#endif
