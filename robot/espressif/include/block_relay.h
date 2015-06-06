#ifndef __block_relay_h_
#define __block_relay_h_
/** @file Manage relaying messages to the blocks
 * @author Daniel Casner
 */

#include "client.h" // Just include client.h since it gets what we want

/// Block ID for no block
#define NO_BLOCK -1
/// Block ID for broadcast to all blocks
#define ALL_BLOCKS 127

/// Maximum length for block messages
#define MAX_BLOCK_MESSAGE_LENGTH 256

/// USER_TASK_PRIO_2 is the highest allowed user task priority USER_TASK_PRIO_0 is the lowest
#define blockTaskPrio USER_TASK_PRIO_1

/// Initalize the block relay module
sint8 blockRelayInit();

/** Request the buffer for sending messages to the specified block
 * @param block The block who's buffer is desired
 * @return A pointer to the start of the buffer to write data into if available. Will return NULL if the requested
 * buffer is busy or block is invalid.
 */
uint8* blockRelayGetBuffer(sint8 block);

/** Send buffered packed to the specified block
 * @param block The ID number of the block to forward to
 * @param len the Number of bytes of data to forward.
 * @return True on success or false on an error.
 */
bool blockRelaySendPacket(sint8 block, unsigned short len);

#endif
