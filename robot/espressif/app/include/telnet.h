/** @file A debug interface and possibly future CLI socket for the robot
 * @author Daniel Casner <daniel@anki.com>
 */
#ifndef __telnet_h
#define __telnet_h

/// Port the espressif will listen on for upgrade commands
#define TELNET_PORT 23
/// The maximum size of a single print call
#define TELNET_MAX_PRINT_LENGTH 1024

/// Initalize the telnet socket
int8_t telnetInit(void);

/// Returns whether a client is currently connected
bool telnetIsConnected(void);

/** Send formatted data to the telnet client (if one is connected)
 * The formatted message is also send out the debug serial port
 * @warning The expanded formatted string must be less than TELNET_MAX_PRINT_LENGTH bytes long
 * @param format A format string to send
 * @return true if the message was successfully sent (or if no client is connected) or false only in an error case
 */
bool telnetPrintf(const char *format, ...);

#endif
