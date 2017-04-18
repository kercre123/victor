#ifndef __RADIO_H
#define __RADIO_H

// Put the radio into a specific test mode
void SetRadioMode(char mode, bool forceupdate );
// Process incoming bytes from the radio - must call at least 12,000 times/second
void RadioProcess();

#endif
