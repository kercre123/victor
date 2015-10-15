/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "telnet.h"

//#define DEBUG_CLIENT

/// Client timeout, 6 seconds
#define CLIENT_TIMEOUT_US 6000000 

static struct espconn *udpServer;
static uint32 lastClientTime = 0x80000000; // ensure first call will result in old result
static esp_udp clientInfo;

#define clientsDiffer(a, b) ((a.remote_port != b.remote_port) || (a.remote_ip[3] != b.remote_ip[3]))

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  const uint32 now = system_get_time();
  struct espconn* conn = (struct espconn*)arg;
  if (conn != udpServer)
  {
    telnetPrintf("Client receive unexpected arg %08x\r\n", arg);
  }

  if ((now - lastClientTime) > CLIENT_TIMEOUT_US)
  {
#ifdef DEBUG_CLIENT
    telnetPrintf("New client from %d.%d.%d.%d:%d\r\n", udpServer->proto.udp->remote_ip[0], udpServer->proto.udp->remote_ip[1], udpServer->proto.udp->remote_ip[2], udpServer->proto.udp->remote_ip[3], udpServer->proto.udp->remote_port);
#endif
    os_memcpy(&clientInfo, conn->proto.udp, sizeof(esp_udp));
  }
  else if (clientsDiffer((*(conn->proto.udp)), clientInfo)) // Existing connection not timed out and packet from a different connection
  {
#ifdef DEBUG_CLIENT
#define newInfo (*(conn->proto.udp))
    telnetPrintf("Ignoring traffic from other source\r\n");
    telnetPrintf("E=%d.%d.%d.%d:%d->%d\tN=%d.%d.%d.%d:%d->%d\r\n",
                 clientInfo.remote_ip[0], clientInfo.remote_ip[1], clientInfo.remote_ip[2], clientInfo.remote_ip[3], clientInfo.remote_port, clientInfo.local_port,
                 newInfo.remote_ip[0], newInfo.remote_ip[1], newInfo.remote_ip[2], newInfo.remote_ip[3], newInfo.remote_port, newInfo.local_port);
#endif
    return; // Ignore the traffic
  }
  lastClientTime = now;
  uartQueuePacket((uint8*)usrdata, len); // Pass to M4
}


sint8 ICACHE_FLASH_ATTR clientInit()
{
  int8 err;

  os_printf("clientInit\r\n");
  
  os_memset(&clientInfo, 0, sizeof(esp_udp));

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

  err = espconn_create(udpServer);
  if (err != 0)
  {
    os_printf("\tError creating server %d\r\n", err);
    return err;
  }

  os_printf("\tno error\r\n");
  return ESPCONN_OK;
}

inline bool clientQueuePacket(uint8* data, uint16 len)
{
#ifdef DEBUG_CLIENT_VERBOSE
telnetPrintf("clientQueuePacket\r\n");
#endif

  if(clientInfo.remote_port != 0)
  {
    const int8 err = espconn_send(udpServer, data, len);
    if (err < 0) // XXX I think a negative number is an error. 0 is OK, I don't know what positive numbers are
    {
      telnetPrintf("Failed to queue UDP packet %d\n", err);
      return false;
    }
    else
    {
      return true;
    }
  }
  else
  {
    return false;
  }
}
