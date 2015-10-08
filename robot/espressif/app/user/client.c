/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "driver/uart.h"
#include "utilEmbedded/transport/IReceiver.h"
#include "utilEmbedded/transport/reliableTransport.h"

//#define DEBUG_CLIENT

#define printf os_printf

static ReliableConnection connection;
static bool haveClient;

bool clientConnected(void)
{
  return haveClient;
}

void clientUpdate(void)
{
  if (ReliableTransport_Update(&connection) == false)
  {
    Receiver_OnDisconnect(&connection);
    printf("Client reliable transport timed out\r\n");
  }
}

static void ICACHE_FLASH_ATTR udpServerRecvCB(void *arg, char *usrdata, unsigned short len)
{
  u16 res;
  
  if (arg != connection.dest)
  {
    printf("Client receive unexpected arg %08x\r\n", (unsigned int)arg);
  }

#ifdef DEBUG_CLIENT
  //printf("udpServerRecvCB %02x[%d] bytes\n", usrdata[0], len);
  if (haveClient == false)
  {
    printf("Client from %d.%d.%d.%d:%d\r\n", udpServer->proto.udp->remote_ip[0], udpServer->proto.udp->remote_ip[1], udpServer->proto.udp->remote_ip[2], udpServer->proto.udp->remote_ip[3], udpServer->proto.udp->remote_port);
  }
#endif

  res = ReliableTransport_ReceiveData(&connection, (uint8_t*)usrdata, len);
  if (res < 0)
  {
    printf("ReliableTransport didn't accept data: %d\r\n", res);
  }
}

sint8 ICACHE_FLASH_ATTR clientInit()
{
  int8 err;

  printf("clientInit\r\n");

  struct espconn *udpServer;

  udpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
  ets_memset( udpServer, 0, sizeof( struct espconn ) );
  udpServer->type = ESPCONN_UDP;
  udpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  ets_memset(udpServer->proto.udp, 0, sizeof(esp_udp));
  udpServer->proto.udp->local_port = 5551;

  err = espconn_regist_recvcb(udpServer, udpServerRecvCB);
  if (err != 0)
  {
    printf("\tError registering receive callback %d\r\n", err);
    return err;
  }

  err = espconn_create(udpServer);
  if (err != 0)
  {
    printf("\tError creating server %d\r\n", err);
    return err;
  }
  
  ReliableTransport_Init();
  ReliableConnection_Init(&connection, udpServer);

  printf("\tno error\r\n");
  return ESPCONN_OK;
}

bool UnreliableTransport_SendPacket(uint8* data, uint16 len)
{
#ifdef DEBUG_CLIENT
printf("clientQueuePacket\n");
#endif

  const int8 err = espconn_send(connection.dest, data, len);
  if (err < 0) // XXX I think a negative number is an error. 0 is OK, I don't know what positive numbers are
  {
    printf("Failed to queue UDP packet %d\n", err);
    return false;
  }
  else
  {
    return true;
  }
}

bool clientSendMessage(const u8* buffer, const u16 size, const u8 msgID, const bool reliable, const bool hot)
{
  if (clientConnected())
  {
    if (reliable)
    {
      if (ReliableTransport_SendMessage(buffer, size, &connection, eRMT_SingleReliableMessage, hot, msgID) == false) // failed to queue reliable message!
      {
        ReliableTransport_Disconnect(&connection);
        Receiver_OnDisconnect(&connection);
        return false;
      }
      else
      {
        return true;
      }
    }
    else
    {
      return ReliableTransport_SendMessage(buffer, size, &connection, eRMT_SingleUnreliableMessage, hot, msgID);
    }
  }
  else
  {
    return false;
  }
}
