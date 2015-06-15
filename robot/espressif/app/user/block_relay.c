
#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "block_relay.h"

//#define DEBUG_BR

#define NUM_BLOCKS 5
static struct espconn* blockServer;

#define RELAY_PORT 6001
#define BLOCK_PORT 6002

#define MESSAGE_START 8

#define    blockTaskQueueLen    10
os_event_t blockTaskQueue[blockTaskQueueLen];

static volatile os_timer_t timer;

typedef struct _BlockStruct
{
  uint8 buffer[MAX_BLOCK_MESSAGE_LENGTH + MESSAGE_START];
  unsigned short length;
  bool  locked;
} Block;

Block blocks[NUM_BLOCKS];

typedef enum {
  BTSig_toBlock
} BlockTaskSignal;



static void ICACHE_FLASH_ATTR blockRecvCB(void *arg, char *usrdata, unsigned short len)
{
  //struct espconn* socket = (struct espconn*)arg;
  //os_printf("BR recv %08x %d[%d]\r\n", arg, usrdata[0], len);
}

LOCAL void ICACHE_FLASH_ATTR blockTask(os_event_t *event)
{
  int8 err = 0;
  sint8 block = event->par;
  int8 i;

  if (block < 0 || block > NUM_BLOCKS)
  {
    os_printf("ERROR: Block task called with invalid block %d\r\n", block);
    return;
  }

  if (blocks[block].locked)
  {
    os_printf("INFO: Skipping message to block %d because locked\r\n", block);
    return;
  }

  blocks[block].locked = true;

  if (blocks[block].length > 0) // have message for block
  {
    blockServer->proto.udp->remote_port = BLOCK_PORT;
    blockServer->proto.udp->remote_ip[0] = 172;
    blockServer->proto.udp->remote_ip[1] = 31;
    blockServer->proto.udp->remote_ip[2] = 1;
    blockServer->proto.udp->remote_ip[3] = 10+block; // Set the block address
    // Send out the packet
    err = espconn_sent(blockServer, blocks[block].buffer, blocks[block].length);
    if (err < 0) /// XXXX I think negative number is an error. 0 is OK, I don't know what positive numbers are
    {
      os_printf("Failed to queue packet to send to block %d: %d\r\n", block, err);
    }
    else
    {
#ifdef DEBUG_BR
      os_printf("Sent %d[%d] to %d at %d\r\n", blocks[block].buffer[MESSAGE_START], blocks[block].length, block, blockServer->proto.udp->remote_ip[3]);
#endif
    }
  }
  blocks[block].locked = false;

#ifdef DEBUG_BR
  //os_printf("BReot %d %d %d %d\r\n", event->sig, event->par, blocks[block].length, err);
#endif
}

static void ICACHE_FLASH_ATTR blockTimer()
{
  // Periodically post tasks to send messages to all blocks
  int8 i;
  for (i=0; i<NUM_BLOCKS; ++i)
  {
    system_os_post(blockTaskPrio, BTSig_toBlock, i);
  }
}


sint8 ICACHE_FLASH_ATTR blockRelayInit()
{
  struct ip_info ipconfig;
  sint8 err, i;

  os_printf("Block Relay Init\r\n");

  system_os_task(blockTask, blockTaskPrio, blockTaskQueue, blockTaskQueueLen); // Setup task queue for handling UART

  for (i=0; i<NUM_BLOCKS; ++i)
  {
    blocks[i].buffer[0] = 0xaa;
    blocks[i].buffer[1] = 0xaa;
    blocks[i].buffer[2] = 0xbe;
    blocks[i].buffer[3] = 0xef;
    blocks[i].buffer[4] = 0x00;
    blocks[i].buffer[5] = 0x00;
    blocks[i].buffer[6] = 0x00;
    blocks[i].buffer[7] = 0x00;
    blocks[i].length = 0;
    blocks[i].locked = false;
  }

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

  // Set block broadcast timer
  os_timer_disarm(&timer);
  os_timer_setfn(&timer, blockTimer);
  os_timer_arm(&timer, 99, true);

  os_printf("\tNo error\r\n");
  return ESPCONN_OK;
}

uint8* ICACHE_FLASH_ATTR blockRelayGetBuffer(sint8 block)
{
  if (block < 0 || block > NUM_BLOCKS)
  {
    os_printf("WARN: invalid block buffer, %d, requested\r\n", block);
    return NULL;
  }
  else if (blocks[block].locked)
  {
    os_printf("INFO: Locked block buffer, %d, requested\r\n", block);
    return NULL;
  }
  else
  {
    blocks[block].locked = true;
    return blocks[block].buffer + MESSAGE_START;
  }
}

bool ICACHE_FLASH_ATTR blockRelaySendPacket(sint8 block, unsigned short len)
{
  if (block < 0 || block > NUM_BLOCKS)
  {
    os_printf("ERROR: Invalid block, %d, specified for send packet\r\n", block);
    return false;
  }
  else
  {
    blocks[block].buffer[4] = (len & 0xff);
    blocks[block].buffer[5] = (len >> 8) & 0xff;
    blocks[block].length = len + MESSAGE_START;
    blocks[block].locked = false;
    system_os_post(blockTaskPrio, BTSig_toBlock, block);
    return true;
  }
}
