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
static bool haveClient = false;

static volatile UDPPacket* queuedPacket; /// Packet that is queued to send
static volatile uint8 nextReserve; /// Index of next buffer to reserve
static volatile uint8 queuedPacketCount; /// Number of packets which have been queued but not sent yet

static void udpServerSentCB(void * arg)
{

#ifdef DEBUG_CLIENT
  os_printf("cl scb qpc=%d\n", queuedPacketCount);
#endif
  os_intr_lock(); // Hopefully this is enough to prevent re-entrance issues with clientQueuePacket
  if (queuedPacketCount > 0)
  {
    queuedPacketCount -= 1;
  }
  else
  {
    os_printf("WARN: udp sent callback with no packet queued\n");
  }
  os_intr_unlock();

}

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  if (arg != (void*)udpServer)
  {
    os_printf("Client receive unexpected arg %08x\r\n", arg);
  }

#ifdef DEBUG_CLIENT
  //os_printf("udpServerRecvCB %02x[%d] bytes\n", usrdata[0], len);
  if (haveClient == false)
  {
    os_printf("Client from %d.%d.%d.%d:%d\r\n", udpServer->proto.udp->remote_ip[0], udpServer->proto.udp->remote_ip[1], udpServer->proto.udp->remote_ip[2], udpServer->proto.udp->remote_ip[3], udpServer->proto.udp->remote_port);
  }
#endif

  haveClient = true;

  uartQueuePacket(usrdata, len); // Pass to M4
}


sint8 ICACHE_FLASH_ATTR clientInit()
{
  int8 err, i;

  os_printf("clientInit\r\n");

  haveClient = false;

  queuedPacket = NULL;
  nextReserve  = 0;
  queuedPacketCount = 0;

  os_memset(rtxbs, 0, sizeof(UDPPacket)*NUM_RTX_BUFS);

  udpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ets_memset( udpServer, 0, sizeof( struct espconn ) );
  udpServer->type = ESPCONN_UDP;
  udpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  ets_memset(udpServer->proto.udp, 0, sizeof(esp_udp));
  udpServer->proto.udp->local_port = 5551;

  err = espconn_regist_recvcb(udpServer, udpServerRecvCB);
  if (err != 0)
  {
    os_printf("\tError registering receive callback %d\r\n", err);
    return err;
  }
  err = espconn_regist_sentcb(udpServer, udpServerSentCB);
  if (err != 0)
  {
    os_printf("\tError registering sent callback %d\r\n", err);
    return err;
  }

  err = espconn_create(udpServer);
  if (err != 0)
  {
    os_printf("\tError creating server %d\r\n", err);
    return err;
  }

  os_printf("\tno error\r\n");
  return ESPCONN_OK;
}


UDPPacket* clientGetBuffer()
{
  UDPPacket* ret = NULL;
#ifdef DEBUG_CLIENT
  os_printf("clientGetBuffer\n");
#endif

  if (haveClient)
  {
    os_intr_lock();
    if (rtxbs[nextReserve].state == PKT_BUF_AVAILABLE)
    {
      rtxbs[nextReserve].state = PKT_BUF_RESERVED;
      ret = &(rtxbs[nextReserve]);
      nextReserve = (nextReserve + 1) % NUM_RTX_BUFS;
    }
    os_intr_unlock();
#ifdef DEBUG_CLIENT
    if (ret == NULL)
    {
      os_printf("\tfailed to reserve packet. nr=%d, [0]%x, [1]%x, [2]%x, [3]%x, qp=%x\n", nextReserve, rtxbs[0].state, rtxbs[1].state, rtxbs[2].state, rtxbs[3].state, queuedPacket);
    }
#endif
  }

  return ret;
}

void clientQueuePacket(UDPPacket* pkt)
{
  uint8 i;
  int8 err;
#ifdef DEBUG_CLIENT
  os_printf("clientQueuePacket\n");
#endif

  os_intr_lock();
  if (pkt->state != PKT_BUF_RESERVED)
  {
    os_printf("Invalid pkt to queue. %d[%d] state=%d at %08x\r\n", pkt->data[0], pkt->len, pkt->state, pkt);
  }
  else if (queuedPacketCount >= 16) {
    os_printf("cl %d too many pkts Qed\r\n", queuedPacketCount);
    pkt->state = PKT_BUF_AVAILABLE;
  }
  else
  {
    err = espconn_sent(udpServer, pkt->data, pkt->len);
    if (err < 0) // XXX I think a negative number is an error. 0 is OK, I don't know what positive numbers are
    {
      os_printf("Failed to queue UDP packet %d\n", err);
    }
    else
    {
      queuedPacketCount += 1;
    }
    pkt->state = PKT_BUF_AVAILABLE;
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
