/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"

//#define DEBUG_CLIENT

#define NUM_RTX_BUFS 4
static UDPPacket rtxbs[NUM_RTX_BUFS];

static struct espconn *udpServer;
static struct espconn *client;

static UDPPacket* queuedPacket; /// Packet that is queued to send
static uint8 nextReserve; /// Index of next buffer to reserve

static void udpServerSentCB(void * arg)
{
#ifdef DEBUG_CLIENT
  os_printf("udpServerSentCB\n");
#endif
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
    os_printf("WARN: udp sent callback with no packet queued\n");
  }
  os_intr_unlock();
}

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  client = (struct espconn *)arg;

#ifdef DEBUG_CLIENT
  os_printf("udpServerRecvCB %02x[%d] bytes\n", usrdata[0], len);
#endif

  espconn_regist_sentcb(client, udpServerSentCB);

  uartQueuePacket(usrdata, len);
}


sint8 ICACHE_FLASH_ATTR clientInit()
{
  int8 err, i;

  os_printf("clientInit\n");

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
    os_printf("\tError registering callback %d\n", err);
    return err;
  }

  err = espconn_create(udpServer);
  if (err != 0)
  {
    os_printf("\tError creating server %d\n", err);
    return err;
  }

  os_printf("\tno error\n");
  return ESPCONN_OK;
}


UDPPacket* clientGetBuffer()
{
  UDPPacket* ret = NULL;
#ifdef DEBUG_CLIENT
  os_printf("clientGetBuffer\n");
#endif

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
#ifdef DEBUG_CLIENT
  if (ret == NULL)
  {
    os_printf("\tfailed to reserve packet. nr=%d, [0]%x, [1]%x, [2]%x, [3]%x, qp=%x\n", nextReserve, rtxbs[0].state, rtxbs[1].state, rtxbs[2].state, rtxbs[3].state, queuedPacket);
  }
#endif

  return ret;
}

void clientQueuePacket(UDPPacket* pkt)
{
  uint8 i;
  int8 err;
#ifdef DEBUG_CLIENT
  os_printf("clientQueuePacket\n");
#endif

  os_intr_lock(); // Hopefully this is enough to prevent re-entrance issues with udpServerSentCB
  err = espconn_sent(client, pkt->data, pkt->len);
  if (err >= 0) // XXX I think a negative number is an error. 0 is OK, I don't know what positive numbers are
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
    os_printf("Failed to queue UDP packet %d\n", err);
    pkt->state = PKT_BUF_AVAILABLE; // Relese the packet that failed to send
  }
  os_intr_unlock();
}

void clientFreePacket(UDPPacket* pkt)
{
#ifdef DEBUG_CLIENT
  os_printf("clientFreePacket\n");
#endif
  pkt->state = PKT_BUF_AVAILABLE;
}
