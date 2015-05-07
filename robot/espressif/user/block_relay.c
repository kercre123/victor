
#include "block_relay.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"

#define NUM_RTX_BUFS 4;
static UDPPacket rtxbs[NUM_RTX_BUFS];

/// Store everything relivant to a block connection
typedef struct BlockConnection
{
  struct espconn socket; // Must be first member so we can use a pointer to espconn as a pointer to the structure
  UDPPacket* queuedPacket;
} BlockConnection;

#define NUM_BLOCKS 3
static BlockConnection* blocks;

#define BLOCK_PORT 6002

static void ICACHE_FLASH_ATTR blockRecvCB(void *arg, char *usrdata, unsigned short len)
{
  espconn* socket = (espconn*)arg;

  UDPPacket* clientPkt = clientGetBuffer();
  if (clientPkt != NULL)
  {
    os_memcpy(clientPkt->data, userdata, len);
    clientQueuePacket(clientPkt);
  }
  else
  {
    os_printf("Block relay couldn't get client buffer to forward packet, dropping %d[%d]\r\n", userdata[0], len);
  }
}

satic void blockSentCB(void *arg)
{
  BlockConnection* block = (BlockConnection*)arg;

  if (block->queuedPacket != NULL) // Reset the packet so it may be used
  {
    UDPPacket* next = block->queuedPacke->next;
    block->queuedPacket->next = NULL;
    block->queuedPacket->len = 0;
    block->queuedPacket->state = PKT_BUF_AVAILABLE;
    block->queuedPacket = next;
  }
  else
  {
    os_printf("WARN: block sent callback with no packet queued\r\n");
  }
}

sint8 ICACHE_FLASH_ATTR blockRelayInit()
{
  sint8 err, i;

  os_printf("Block Relay Init\r\n");

  os_memset(rtxbs, 0, sizeof(UDPPacket)*NUM_RTX_BUFS);

  blocks = (BlockConnection*)os_zalloc(sizeof(BlockConnection)*NUM_BLOCKS); // Allocate block array
  if (blocks == NULL)
  {
    os_printf("\tCould not allocate memory for block connections\r\n");
    return -100;
  }
  os_memset(blocks, 0, sizeof(BlockConnection)*NUM_BLOCKS);

  for (i=0; i<NUM_BLOCKS; ++i)
  {
    blocks[i].socket.type = ESPCONN_UDP;
    blocks[i].socket.proto.udp = (esp_udp*)os_zalloc(sizeof(esp_udp));
    if (blocks[i].socket.proto.udp == NULL)
    {
      os_printf("\tCould not allocate memory for UDP socket parameters\r\n");
      return -101;
    }
    blocks[i].socket.proto.udp.local_port = espconn_port();
    blocks[i].socket.proto.udp.remote_port = BLOCK_PORT;
    blocks[i].socket.proto.udp.remote_ip[0] = 172;
    blocks[i].socket.proto.udp.remote_ip[1] = 31;
    blocks[i].socket.proto.udp.remote_ip[2] = 1;
    blocks[i].socket.proto.udp.remote_ip[3] = i + 10;
    err = espconn_regist_recvcb(&blocks[i].socket, blockRecvCB);
    if (err != 0)
    {
      os_printf("\tError registering receive callback for block %d: %d\r\n", i, err);
      return err;
    }
    err = espconn_regist_sentcb(&blocks[i].socket, blockSentCB);
    if (err != 0)
    {
      os_printf("\tError registering sent callback for block %d: %d", i, err);
    }
    err = espconn_create(&blocks[i].socket);
    if (err != 0)
    {
      os_printf("\tError creating socket for block %d: %d\r\n", i, err);
      return err;
    }

    os_printf("\tNo error\r\n");
    return ESPCONN_OK;
  }
}

sint8 blockRelayCheckMessage(uint8* data, unsigned short len)
{
  switch (data[0])
  {
    case 62:
    {
      return ALL_BLOCKS; // Broadcast
    }
    case 63:
    {
      if (len == 26)
      {
        return data[25];
      }
      else
      {
        os_printf("Got SetBlockLights (%d), expected %d bytes, but got %d\r\n", data[0], 26, len);
        return NO_BLOCK;
      }
      break;
    }
    default
    {
      return NO_BLOCK;
    }
  }
}

bool blockRelaySendPacket(sint8 block, uint8* data, unsigned short len)
{
  sint8 i;
  bool success;
  if (block == NO_BLOCK)
  {
    os_printf("Block relay send packet called with NO_BLOCK\r\n");
    return false;
  }
  else if(block == ALL_BLOCKS)
  {
    for (i=0; i<NUM_BLOCKS; ++i)
    {
      success = blockRelaySendPacket(i, data, len);
      if (success == false)
      {
        os_printf("Failed to broadcast %d[%d] to block %d\r\n", data[0], len, i);
        return false;
      }
    }
  }
  else if(block < NUM_BLOCKS)
  {
    for (i=0; i<NUM_RTX_BUFS; ++i)
    {
      if (rtxbs[i].state == PKT_BUF_AVAILABLE)
      {
        int8 err;
        rtxbs[i].state = PKT_BUF_RESERVED;
        os_memcpy(rtxbs[i].data, data, len);
        rtxbs[i].len = len;
        err = espconn_sent(blocks[block].socket, rtxbs[i].data, rtxbs[i].len);
        if (err >= 0) /// XXXX I think negative number is an error. 0 is OK, I don't know what positive numbers are
        {
          rtxbs[i].state = PKT_BUF_QUEUED;
          if (blocks[block].queuedPacket == NULL)
          {
            blocks[block].queuedPacket = &rtxbs[i];
          }
          else
          {
            UDPPacket* n = blocks[block].queuedPacket;
            while (n->next != NULL) n = n->next;
            n->next = &rtxbs[i];
          }
          return true;
        }
        else
        {
          os_printf("Failed to queue packet to send to block %d: %d\r\n", block, err);
          return false;
        }
      }
    }
    os_printf("Block relay no transmit buffer availalbe to relay %d[%d] to %d\r\n", data[0], len, block);
    return false;
  }
  else
  {
    os_printf("Block relay send packet called for block %d but only %d blocks\r\n", block, NUM_BLOCKS);
    return false;
  }
}
