// This documents the protocol used by the robot/accessory (R2A and A2R)
#ifndef MESSAGES_H__
#define MESSAGES_H__

#define ADV_CHANNEL   81    // 2481MHz
#define ADV_LEN       10    // 10 byte advertising/sync packets

#define HAND_LEN      17    // 17 byte handshake packets

#define R2A_BASIC_SETLEDS   0     // Set the LEDs and get basic IMU/tap responses
#define R2A_OTA             0xF0  // 0xF0..0xFF: Send OTA data - ROBOT MUST REPEAT EACH OTA MESSAGE UNTIL ACKED

#define A2R_SIMPLE_TAP      0     // "Classic" tap message - XYZ + tap count

#endif
