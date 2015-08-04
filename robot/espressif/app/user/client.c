/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "telnet.h"

//#define DEBUG_CLIENT

static struct espconn *udpServer;
static bool haveClient = false;

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  if (arg != (void*)udpServer)
  {
    telnetPrintf("Client receive unexpected arg %08x\r\n", arg);
  }

#ifdef DEBUG_CLIENT
  //os_printf("udpServerRecvCB %02x[%d] bytes\n", usrdata[0], len);
  if (haveClient == false)
  {
    telnetPrintf("Client from %d.%d.%d.%d:%d\r\n", udpServer->proto.udp->remote_ip[0], udpServer->proto.udp->remote_ip[1], udpServer->proto.udp->remote_ip[2], udpServer->proto.udp->remote_ip[3], udpServer->proto.udp->remote_port);
  }
#endif

  haveClient = true;

  uartQueuePacket((uint8*)usrdata, len); // Pass to M4
}


sint8 ICACHE_FLASH_ATTR clientInit()
{
  int8 err;

  os_printf("clientInit\r\n");

  haveClient = false;

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
#ifdef DEBUG_CLIENT
telnetPrintf("clientQueuePacket\n");
#endif

  if(haveClient)
  {
    const int8 err = espconn_sent(udpServer, data, len);
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
