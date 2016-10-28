/** @file Wrapper for UDP client connections
 * @author Daniel Casner
 */

#include "client.h"
#include "ip_addr.h"
#include "espconn.h"
#include "mem.h"
#include "ets_sys.h"
#include "osapi.h"
#include "backgroundTask.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "anki/cozmo/transport/IReceiver.h"
#include "anki/cozmo/transport/reliableTransport.h"
#include "driver/crash.h" 


//#define DEBUG_CLIENT 1

#define printf os_printf

#define destsEqual(a, b) (((a)->remote_port == (b)->remote_port) && ((a)->remote_ip[3] == (b)->remote_ip[3]))

static struct espconn* socket;
static ReliableConnection* clientConnection;
static uint32_t clientConnectionId;
static uint32_t dropCount;
static bool accept;
static bool sendHoldoff;
static uint32_t reliable_disconnect_timestamp;
static const uint32_t kWifiReconnect_timeout_us = 30000000;  //30 sec

bool clientConnected(void)
{
  return clientConnectionId != 0;
}

void clientAccept(const bool enabled)
{
  accept = enabled;
}

sint16 clientQueueAvailable(void)
{
  if (clientConnection) return ReliableConnection_GetReliableQueueAvailable(clientConnection);
  else return 0;
}

uint32 clientDropCount(void)
{
  return dropCount;
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
  else if (reliable_disconnect_timestamp) //we were connected but now are not.
  {
    uint32_t currentTime = system_get_time();
    if ((currentTime - reliable_disconnect_timestamp) > kWifiReconnect_timeout_us)
    {
      os_printf("WIFI is connected but RT droppped. Resetting\r\f");
      wifi_set_opmode_current(NULL_MODE);  //off
      wifi_set_opmode_current(SOFTAP_MODE); //ap-mode back on.
      reliable_disconnect_timestamp = 0; //only once
    }
  }
}


void ExtraConnection_SendDisconnect_Maybe(const remot_info* remote, const uint8_t* data_in, unsigned short len)
{
  //we can't use the RT functions without messing up their sequencing.
  //so build a disconnect message from raw bytes.
  uint8_t data[14] = { 'C','O','Z', 3, 'R','E',1,
                        eRMT_DisconnectRequest, 1,0,1,0,1, 0};

  if (len>7) { len=7; } //only compare beginning of header: "COZ\3RE\1"
  if (!os_memcmp(data, data_in, len))
  {
     return;   //don't reply unless incoming message had RT header.
  }
  //reply to sender
  os_memcpy(socket->proto.udp->remote_ip, remote->remote_ip, 4);
  socket->proto.udp->remote_port = remote->remote_port;
  
#ifdef DEBUG_CLIENT
  printf("Sending Disconnect Directly to to %d.%d.%d.%d:%d\r\n", socket->proto.udp->remote_ip[0], socket->proto.udp->remote_ip[1],
         socket->proto.udp->remote_ip[2], socket->proto.udp->remote_ip[3], socket->proto.udp->remote_port);
#endif

  const int8 err = espconn_send(socket, data, sizeof(data));
  if (err < 0)
  {
    printf("ECSD %d\r\n", err);
  }
}


ReliableConnection g_conn;   // So we can check canaries when we crash
static void socketRecvCB(void *arg, char *usrdata, unsigned short len)
{
  struct espconn* src = (struct espconn*)arg;
  static esp_udp s_dest;
  remot_info *remote = NULL;
  ISR_STACK_LEFT('C');

  sendHoldoff = false;

  sint8 err;
  err = espconn_get_connection_info(src,&remote,0);
  if (err != ESPCONN_OK)
  {
     os_printf("ERROR, couldn't get remote info for connection! %d\r\n", err);
     recordBootError(espconn_get_connection_info, err);
     return;
  }
  
  if (unlikely(!clientConnected()))
  {
    
#ifdef DEBUG_CLIENT
    printf("Initalizing new connection from  %d.%d.%d.%d:%d\r\n", remote->remote_ip[0], remote->remote_ip[1], remote->remote_ip[2], remote->remote_ip[3], remote->remote_port);
#endif    
    esp_udp* dest    = &s_dest;
    clientConnection = &g_conn;
    os_memcpy(dest->remote_ip, remote->remote_ip, 4);
    dest->remote_port = remote->remote_port;
    ReliableConnection_Init(clientConnection, dest);
    
  }
  else if (unlikely(!destsEqual(remote, (esp_udp*)clientConnection->dest)))
  {
#ifdef DEBUG_CLIENT
    printf("Ignoring UDP traffic from unconnected source at  %d.%d.%d.%d:%d\r\n", remote->remote_ip[0], remote->remote_ip[1], remote->remote_ip[2], remote->remote_ip[3], remote->remote_port);
#endif
    ExtraConnection_SendDisconnect_Maybe(remote, (uint8_t*)usrdata, len);
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
  static struct espconn s_socket;
  static esp_udp s_udp;
  
  //printf("clientInit\r\n");

  clientConnection = NULL;
  clientConnectionId = 0;
  dropCount = 0;
  accept = false;
  sendHoldoff = false;


  socket = &s_socket;
  os_memset( socket, 0, sizeof( struct espconn ) );
  socket->type = ESPCONN_UDP;
  socket->proto.udp = &s_udp;
  os_memset(socket->proto.udp, 0, sizeof(esp_udp));
  socket->proto.udp->local_port = 5551;

  err = espconn_regist_recvcb(socket, socketRecvCB);
  if (err != 0)
  {
    printf("\tError registering receive callback %d\r\n", err);
    recordBootError((void*)espconn_regist_recvcb, err);
    return err;
  }

  err = espconn_create(socket);
  if (err != 0)
  {
    printf("\tError creating server %d\r\n", err);
    recordBootError(espconn_create, err);
    return err;
  }
    
  ReliableTransport_Init();

  //printf("\tno error\r\n");
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
        dropCount++;
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
  const esp_udp* const dest = (esp_udp*)clientConnection->dest;
  os_memcpy(socket->proto.udp->remote_ip, dest->remote_ip, 4);
  socket->proto.udp->remote_port = dest->remote_port;
  
  
#ifdef DEBUG_CLIENT
  printf("clientQueuePacket to %d.%d.%d.%d:%d\r\n", socket->proto.udp->remote_ip[0], socket->proto.udp->remote_ip[1],
                    socket->proto.udp->remote_ip[2], socket->proto.udp->remote_ip[3], socket->proto.udp->remote_port);
#endif

  if (sendHoldoff)
  {
    return false;
  }
  else
  {
    const int8 err = espconn_send(socket, data, len);
    if (err < 0) // I think a negative number is an error. 0 is OK, I don't know what positive numbers are
    {
      printf("FQUP %d\r\n", err);
      recordBootError(espconn_send, err);
      sendHoldoff = true;
      return false;
    }
    else
    {
      return true;
    }
  }
}

void Receiver_OnConnectionRequest(ReliableConnection* conn)
{
  if (accept)
  {
    printf("New Reliable transport connection request\r\n");
    if (clientConnectionId == 0) // Not connected yet
    {
      ReliableTransport_FinishConnection(conn); // Accept the connection
      clientConnectionId = 1; // Eventually we'll get this from the connection request or finished message
      dropCount = 0;
    }
    else // The engine is trying to reconnect
    {
      printf("\tlooks like a new engine, clearing current connection\r\n");
      Receiver_OnDisconnect(conn); // Kill our connection instance so we can make a new one with the new engine instance
    }
  }
  else printf("Not accepting reliable transport connection\r\n");
}

void Receiver_OnConnected(ReliableConnection* conn)
{
  printf("Reliable transport connection completed\r\n");
  reliable_disconnect_timestamp = 0; //we are connected;
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
    if (conn != NULL)
    {
      reliable_disconnect_timestamp = system_get_time()|1;  //make sure it is always non-zero if we are timing
    }
    clientConnection = NULL;
    clientConnectionId = 0;
  }
}

// Forward declaration of externally defined function
void ProcessMessage(u8* buffer, u16 bufferSize);

void Receiver_ReceiveData(uint8_t* buffer, uint16_t bufferSize, ReliableConnection* conn)
{
  ProcessMessage(buffer, bufferSize);
}
