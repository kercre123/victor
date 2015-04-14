/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"

//#define DEBUG_PRINT

#ifdef DEBUG_PRINT
#include "driver/uart.h"
#define user_print(txt) uart0_tx_buffer(txt, os_strlen(txt))
#else
#define user_print(txt) //
#endif


#define NUM_RTX_BUFS 4
static UDPPacket rtxbs[NUM_RTX_BUFS];

static struct espconn *udpServer;
static struct espconn *client;

static UDPPacket* queuedPacket; /// Packet that is queued to send
static uint8 nextReserve; /// Index of next buffer to reserve

static clientReceiveCB userRecvCB;

static void udpServerSentCB(void * arg)
{
  user_print("udpServerSentCB\r\n");
  os_intr_lock(); // Hopefully this is enough to prevent re-entrance issues with clientQueuePacket
  if (queuedPacket != NULL)
  {
    UDPPacket* next = queuedPacket->next;
    queuedPacket->next = NULL;
    queuedPacket->state = PKT_BUF_AVAILABLE;
    queuedPacket = next;
  }
  else
  {
    user_print("\tno queued packet!\r\n");
  }
  os_intr_unlock();
}

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  char err = 0;
  client = (struct espconn *)arg;

  user_print("udpServerRecvCB\r\n");

  espconn_regist_sentcb(client, udpServerSentCB);

  userRecvCB(usrdata, len);
}


sint8 ICACHE_FLASH_ATTR clientInit(clientReceiveCB recvFtn)
{
  int8 err, i;

  user_print("clientInit\r\n");

  userRecvCB = recvFtn;

  client = NULL;

  queuedPacket = NULL;
  nextReserve  = 0;

  os_memset(rtxbs, 0, sizeof(UDPPacket)*NUM_RTX_BUFS);

  udpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ets_memset( udpServer, 0, sizeof( struct espconn ) );
  espconn_create( udpServer );
  udpServer->type = ESPCONN_UDP;
  udpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  udpServer->proto.udp->local_port = 5551;

  err = espconn_regist_recvcb(udpServer, udpServerRecvCB);
  if (err != 0)
  {
    user_print("Error registering callback\r\n");
    return err;
  }

  err = espconn_create(udpServer);
  if (err != 0)
  {
    user_print("Error creating server\r\n");
    return err;
  }

  user_print("\tno error\r\n");
  return ESPCONN_OK;
}


UDPPacket* clientGetBuffer()
{
  UDPPacket* ret = NULL;
  user_print("clientGetBuffer\r\n");

  if (client != NULL)
  {
    os_intr_lock();
    if (rtxbs[nextReserve].state == PKT_BUF_AVAILABLE)
    {
      rtxbs[nextReserve].state = PKT_BUF_RESERVED;
      ret = &(rtxbs[nextReserve]);
      nextReserve = (nextReserve + 1) % NUM_RTX_BUFS;
    }
    os_intr_unlock();
  }
#ifdef DEBUG_PRINT
  if (ret == NULL)
  {
    char txt[200];
    os_sprintf(txt, "\tfailed to reserve packet. nr=%d, [0]%x, [1]%x, [2]%x, [3]%x, qp=%x\r\n", nextReserve, rtxbs[0].state, rtxbs[1].state, rtxbs[2].state, rtxbs[3].state, queuedPacket);
    user_print(txt);
  }
#endif

  return ret;
}

void clientQueuePacket(UDPPacket* pkt)
{
  uint8 i;
  user_print("clientQueuePacket\r\n");

  os_intr_lock(); // Hopefully this is enough to prevent re-entrance issues with udpServerSentCB
  if (espconn_sent(client, pkt->data, pkt->len) == ESPCONN_OK)
  {
    pkt->state = PKT_BUF_QUEUED;
    // Append to queue
    if (queuedPacket == NULL)
    {
      queuedPacket = pkt;
    }
    else
    {
      UDPPacket* n = queuedPacket;
      while (n->next != NULL) n = n->next;
      n->next = pkt;
    }
  }
  else
  {
    user_print("\tFailed to queue packet\r\n");
    pkt->state = PKT_BUF_AVAILABLE; // Relese the packet that failed to send
  }
  os_intr_unlock();
}

void clientFreePacket(UDPPacket* pkt)
{
  user_print("clientFreePacker\r\n");
  pkt->state = PKT_BUF_AVAILABLE;
}
