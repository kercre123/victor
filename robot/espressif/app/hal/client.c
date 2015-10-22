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

#define destsEqual(a, b) (((a)->remote_port == (b)->remote_port) && ((a)->remote_ip[3] == (b)->remote_ip[3]))

static struct espconn* socket;
static ReliableConnection* clientConnection;
static uint32_t clientConnectionId;

bool clientConnected(void)
{
  return clientConnection != NULL;
}

void clientUpdate(void)
{
  if (clientConnection != NULL)
  {
    if (ReliableTransport_Update(clientConnection) == false)
    {
      Receiver_OnDisconnect(clientConnection);
      printf("Client reliable transport timed out\r\n");
    }
  }
}

static void socketRecvCB(void *arg, char *usrdata, unsigned short len)
{
  struct espconn* src = (struct espconn*)arg;
  
  if (unlikely(!clientConnected()))
  {
#ifdef DEBUG_CLIENT
    printf("Initalizing new connection from  %d.%d.%d.%d:%d\r\n", src->proto.udp->remote_ip[0], src->proto.udp->remote_ip[1], src->proto.udp->remote_ip[2], src->proto.udp->remote_ip[3], src->proto.udp->remote_port);
#endif    
    esp_udp* dest    = os_zalloc(sizeof(esp_udp));
    clientConnection = os_zalloc(sizeof(ReliableConnection));
    if (unlikely(clientConnection == NULL || dest == NULL))
    {
      os_printf("DIE! Couldn't allocate memory for reliable connection!\r\n");
      return;
    }
    os_memcpy(dest, src->proto.udp, sizeof(esp_udp));
    ReliableConnection_Init(clientConnection, dest);
  }
  else if (unlikely(!destsEqual(src->proto.udp, (esp_udp*)clientConnection->dest)))
  {
#ifdef DEBUG_CLIENT
    printf("Ignoring UDP traffic from unconnected source at  %d.%d.%d.%d:%d\r\n", src->proto.udp->remote_ip[0], src->proto.udp->remote_ip[1], src->proto.udp->remote_ip[2], src->proto.udp->remote_ip[3], src->proto.udp->remote_port);
#endif
    return;
  }
  
  u16 res = ReliableTransport_ReceiveData(clientConnection, (uint8_t*)usrdata, len);
  if (res < 0)
  {
    printf("ReliableTransport didn't accept data: %d\r\n", res);
  }
}

sint8 clientInit()
{
  int8 err;

  printf("clientInit\r\n");

  clientConnection = NULL;
  clientConnectionId = 0;

  socket = (struct espconn *)os_zalloc(sizeof(struct espconn));
  os_memset( socket, 0, sizeof( struct espconn ) );
  socket->type = ESPCONN_UDP;
  socket->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
  os_memset(socket->proto.udp, 0, sizeof(esp_udp));
  socket->proto.udp->local_port = 5551;

  err = espconn_regist_recvcb(socket, socketRecvCB);
  if (err != 0)
  {
    printf("\tError registering receive callback %d\r\n", err);
    return err;
  }

  err = espconn_create(socket);
  if (err != 0)
  {
    printf("\tError creating server %d\r\n", err);
    return err;
  }
    
  ReliableTransport_Init();

  printf("\tno error\r\n");
  return ESPCONN_OK;
}

bool clientSendMessage(const u8* buffer, const u16 size, const u8 msgID, const bool reliable, const bool hot)
{
  if (likely(clientConnected()))
  {
    if (unlikely(reliable))
    {
      if (unlikely(ReliableTransport_SendMessage(buffer, size, clientConnection, eRMT_SingleReliableMessage, hot, msgID) == false)) // failed to queue reliable message!
      {
        ReliableTransport_Disconnect(clientConnection);
        Receiver_OnDisconnect(clientConnection);
        return false;
      }
      else
      {
        return true;
      }
    }
    else
    {
      return ReliableTransport_SendMessage(buffer, size, clientConnection, eRMT_SingleUnreliableMessage, hot, msgID);
    }
  }
  else
  {
    return false;
  }
}

bool UnreliableTransport_SendPacket(uint8* data, uint16 len)
{
#ifdef DEBUG_CLIENT
  printf("clientQueuePacket to %d.%d.%d.%d:%d\r\n", socket->proto.udp->remote_ip[0], socket->proto.udp->remote_ip[1],
                    socket->proto.udp->remote_ip[2], socket->proto.udp->remote_ip[3], socket->proto.udp->remote_port);
#endif

  const int8 err = espconn_send(socket, data, len);
  if (err < 0) // I think a negative number is an error. 0 is OK, I don't know what positive numbers are
  {
    printf("Failed to queue UDP packet %d\r\n", err);
    return false;
  }
  else
  {
    return true;
  }
}

void Receiver_OnConnectionRequest(ReliableConnection* conn)
{
  printf("New Reliable transport connection request\r\n");
  if (clientConnectionId == 0) // Not connected yet
  {
    ReliableTransport_FinishConnection(conn); // Accept the connection
  }
  else // The engine is trying to reconnect
  {
    printf("\tlooks like a new engine, clearing current connection\r\n");
    Receiver_OnDisconnect(conn); // Kill our connection instance so we can make a new one with the new engine instance
  }
}

void Receiver_OnConnected(ReliableConnection* conn)
{
  printf("Reliable transport connection completed\r\n");
  clientConnectionId = 1; // Eventually we'll get this from the connection request or finished message
}

void Receiver_OnDisconnect(ReliableConnection* conn)
{
  printf("Reliable transport disconnected\r\n");
  if (conn != clientConnection) 
  {
    printf("WTF: Unexpected disconnect argument! %08x != %08x\r\n", (unsigned int)conn, (unsigned int)clientConnection);
  }
  else
  {
    os_free(clientConnection->dest);
    os_free(clientConnection);
    clientConnection = NULL;
    clientConnectionId = 0;
  }
}

void Receiver_ReceiveData(uint8_t* buffer, uint16_t bufferSize, ReliableConnection* conn)
{
  //XXX  cross link to message dispatch
}
