
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "block_relay.h"

#define DEBUG_BR

#define NUM_BLOCKS 3
static struct espconn* blockServer;

#define RELAY_PORT 6001
#define BLOCK_PORT 6002

#define MESSAGE_START 8

#define    blockTaskQueueLen    10
os_event_t blockTaskQueue[blockTaskQueueLen];

static UDPPacket* clientPkt;

static uint8 blockPkt[PKT_BUFFER_SIZE];
static bool  blockPktQueued;

typedef enum
{
  BR_SIG_TO_CLIENT = 1,
  BR_SIG_TO_BLOCK  = 2,
} BlockRelayTaskSignal;


static void ICACHE_FLASH_ATTR blockRecvCB(void *arg, char *usrdata, unsigned short len)
{
  struct espconn* socket = (struct espconn*)arg;

#ifdef DEBUG_BR
  os_printf("BR recv %08x %d[%d]\r\n", arg, usrdata[0], len);
#endif

  if (clientPkt != NULL)
  {
    os_printf("Block relay couldn't forward packet from block because queue full\r\n");
    return;
  }

  clientPkt = clientGetBuffer();

  if (clientPkt != NULL)
  {
    os_memcpy(clientPkt->data, usrdata + MESSAGE_START, len-MESSAGE_START);
    system_os_post(blockTaskPrio, BR_SIG_TO_CLIENT, len);
  }
  else
  {
    os_printf("Block relay couldn't get client buffer to forward packet, dropping %d[%d]\r\n", usrdata[0], len);
  }
}

LOCAL void ICACHE_FLASH_ATTR blockTask(os_event_t *event)
{
  int8 err = 0;

  if (event->sig & BR_SIG_TO_CLIENT) // Have a packet to send
  {
    if (clientPkt == NULL)
    {
      os_printf("Error blockTask sig to client with null pkt\r\n");
    }
    else
    {
      clientPkt->len = event->par;
      clientQueuePacket(clientPkt);
      clientPkt = NULL;
    }
  }
  else if (event->sig & BR_SIG_TO_BLOCK) // Relay to block
  {
    sint8 block = event->sig >> 16;
    int8 i;
    if (block <= NO_BLOCK)
    {
      os_printf("Block relay send packet called with NO_BLOCK\r\n");
      blockPktQueued = false;
    }
    else if(block == ALL_BLOCKS)
    {
      for (i=0; i<NUM_BLOCKS; ++i)
      {
        // Repost task for each individual block
        system_os_post(blockTaskPrio, BR_SIG_TO_BLOCK | (i << 16), event->par);
      }
    }
    else if(block < NUM_BLOCKS)
    {
      blockServer->proto.udp->remote_port = BLOCK_PORT;
      blockServer->proto.udp->remote_ip[0] = 172;
      blockServer->proto.udp->remote_ip[1] = 31;
      blockServer->proto.udp->remote_ip[2] = 1;
      blockServer->proto.udp->remote_ip[3] = 10+block; // Set the block address
#ifdef DEBUG_BR
      os_printf("\tSending to %d bytes to block %d @ %d.%d.%d.%d:%d\r\n", event->par, block, blockServer->proto.udp->remote_ip[0], blockServer->proto.udp->remote_ip[1], blockServer->proto.udp->remote_ip[2], blockServer->proto.udp->remote_ip[3], blockServer->proto.udp->remote_port);
#endif
      // Put the length into the serial header
      blockPkt[4] = (event->par-MESSAGE_START)        & 0xff;
      blockPkt[5] = ((event->par-MESSAGE_START) >> 8) & 0xff;
      // Send out the packet
      err = espconn_sent(blockServer, blockPkt, event->par);
      if (err < 0) /// XXXX I think negative number is an error. 0 is OK, I don't know what positive numbers are
      {
        os_printf("Failed to queue packet to send to block %d: %d\r\n", block, err);
      }
      blockPktQueued = false;
    }
    else
    {
      os_printf("Block relay send packet called for block %d but only %d blocks\r\n", block, NUM_BLOCKS);
      blockPktQueued = false;
    }
  }
  else
  {
    os_printf("ERROR: blockTask with unknown signal %08x\r\n", event->sig);
  }
#ifdef DEBUG_BR
  os_printf("BReot %04x %04x %d %d\r\n", (uint16)(event->sig >> 16), (uint16)(event->sig & 0xff), event->par, err);
#endif
}


sint8 ICACHE_FLASH_ATTR blockRelayInit()
{
  struct ip_info ipconfig;
  sint8 err, i;

  os_printf("Block Relay Init\r\n");

  system_os_task(blockTask, blockTaskPrio, blockTaskQueue, blockTaskQueueLen); // Setup task queue for handling UART

  clientPkt = NULL;
  blockPkt[0] = 0xaa;
  blockPkt[1] = 0xaa;
  blockPkt[2] = 0xbe;
  blockPkt[3] = 0xef;
  blockPkt[4] = 0x00;
  blockPkt[5] = 0x00;
  blockPkt[6] = 0x00;
  blockPkt[7] = 0x00;

  blockPktQueued = false;

  err = wifi_get_ip_info(SOFTAP_IF, &ipconfig);
  if (err == false)
  {
    os_printf("\tCould not get IP info\r\n");
    return -102;
  }

  blockServer = (struct espconn*)os_zalloc(sizeof(struct espconn));
  if (blockServer == NULL)
  {
    os_printf("\tCould not allocate memory for block connection %d\r\n", i);
    return -100;
  }
  ets_memset(blockServer, 0, sizeof(struct espconn));
  blockServer->state = ESPCONN_NONE;
  blockServer->type  = ESPCONN_UDP;
  blockServer->proto.udp = (esp_udp*)os_zalloc(sizeof(esp_udp));
  if (blockServer->proto.udp == NULL)
  {
    os_printf("\tCould not allocate memory for UDP socket parameters for block %d\r\n", i);
    return -101;
  }
  os_memcpy(blockServer->proto.udp->local_ip, &(ipconfig.ip.addr), 4);
  blockServer->proto.udp->local_port = RELAY_PORT;
  blockServer->proto.udp->remote_port = BLOCK_PORT;
  blockServer->proto.udp->remote_ip[0] = 172;
  blockServer->proto.udp->remote_ip[1] = 31;
  blockServer->proto.udp->remote_ip[2] = 1;
  blockServer->proto.udp->remote_ip[3] = 10;

  err = espconn_regist_recvcb(blockServer, blockRecvCB);
  if (err != 0)
  {
    os_printf("\tError registering receive callback for block %d: %d\r\n", i, err);
    return err;
  }
  err = espconn_create(blockServer);
  if (err != 0)
  {
    os_printf("\tError creating socket for block %d: %d\r\n", i, err);
    return err;
  }

  os_printf("\tNo error\r\n");
  return ESPCONN_OK;
}

sint8 blockRelayCheckMessage(uint8* data, unsigned short len)
{
#if 0
  os_printf("BR check message %d[%d]\r\n", data[0], len);
#endif

  switch (data[0])
  {
    case 62:
    {
      return ALL_BLOCKS; // Broadcast
    }
    case 63:
    {
      if (len == 194)
      {
        return data[193];
      }
      else
      {
        os_printf("Got SetBlockLights (%d), expected %d bytes, but got %d\r\n", data[0], 194, len);
        return NO_BLOCK;
      }
      break;
    }
    default:
    {
      return NO_BLOCK;
    }
  }
}

bool ICACHE_FLASH_ATTR blockRelaySendPacket(sint8 block, uint8* data, unsigned short len)
{
  if (blockPktQueued)
  {
    os_printf("WARN: blockRelaySendPacket can't relay because packet already queued\r\n");
    return false;
  }
  else
  {
    os_memcpy(blockPkt + MESSAGE_START, data, len + MESSAGE_START);
    system_os_post(blockTaskPrio, BR_SIG_TO_BLOCK | (block << 16), len + MESSAGE_START);
    return true;
  }
}
