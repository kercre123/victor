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

/// USER_TASK_PRIO_2 is the highest allowed user task priority USER_TASK_PRIO_0 is the lowest
#define blockTaskPrio USER_TASK_PRIO_1

/// Initalize the block relay module
sint8 blockRelayInit();

/** Check a message to see if it should be relayed to a block and if so which one
 * @param data A pointer to the buffer where the message is
 * @param len The number of bytes in data
 * @return The ID number of the block the message should be forwared to (to pass to blockRelaySendPacket) if it should
 * be forwarded or NO_BLOCK if it is not a block message to be relayed.
 */
sint8 blockRelayCheckMessage(uint8* data, unsigned short len);

/** Send a message to the specified block
 * @param block The ID number of the block to forward to
 * @param data A pointer to the data to forward
 * @param len the Number of bytes of data to forward.
 * @return True on success or false on an error.
 */
bool blockRelaySendPacket(sint8 block, uint8* data, unsigned short len);

#endif
