/** @file Telnet debugging server
 * For printing stuff over a socket and maybe sometimes listening
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "espconn.h"
#include "telnet.h"
#include <stdarg.h>

/// This socket server
static struct espconn* telnetServer;

/// The active connection (if any)
static struct espconn* telnetConnection;

LOCAL void ICACHE_FLASH_ATTR telnetReceiveCallback(void *arg, char *usrdata, unsigned short length)
{
  os_printf("Telnet received %d bytes of data:\r\n\t\"%s\"\r\n", length, usrdata);
}

LOCAL void ICACHE_FLASH_ATTR telnetSentCallback(void *arg)
{
  //struct espconn *conn = arg;
}

LOCAL void ICACHE_FLASH_ATTR telnetConnectCallback(void *arg)
{
  struct espconn* conn = (struct espconn*)arg;
  uint8 err;

  if (telnetConnection != NULL)
  {
    os_printf("Telnet new client request while already connected\r\n");
  }
  else
  {
    os_printf("Telnet new client connecting\r\n");
  }
  telnetConnection = conn;

  err = espconn_regist_recvcb(conn, telnetReceiveCallback);
  if (err != 0)
  {
    os_printf("ERROR: telnet couldn't register receive callback: %d\r\n", err);
    return;
  }
  err = espconn_regist_sentcb(conn, telnetSentCallback);
  if (err != 0)
  {
    os_printf("ERROR: telnet couldn't register sent callback: %d\r\n", err);
    return;
  }
}

LOCAL void ICACHE_FLASH_ATTR telnetDisconectCallback(void *arg)
{
  os_printf("Telnet client disconnected\r\n");
  telnetConnection = NULL;
}

LOCAL void ICACHE_FLASH_ATTR telnetReconnectCallback(void *arg, sint8 err)
{
  os_printf("Telnet server error: %d\r\n", err);
  telnetDisconectCallback(arg);
}

int8_t ICACHE_FLASH_ATTR telnetInit(void)
{
  int8_t err;
  os_printf("Telnet init\r\n");

  telnetConnection = NULL;

  telnetServer = (struct espconn*)os_zalloc(sizeof(struct espconn));
  if (telnetServer == NULL)
  {
    os_printf("\tCouldn't allocate memory for the telnet server\r\n");
    return -100;
  }
  os_memset(telnetServer, 0, sizeof(struct espconn));
  telnetServer->state = ESPCONN_NONE;
  telnetServer->type  = ESPCONN_TCP;
  telnetServer->proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
  if (telnetServer->proto.tcp == NULL)
  {
    os_printf("\tCouldn't allocate memory for telnet server TCP parameters\r\n");
    return -101;
  }
  telnetServer->proto.tcp->local_port = TELNET_PORT;

  err = espconn_regist_connectcb(telnetServer, telnetConnectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Telnet registering connect cb: %d\r\n", err);
    return err;
  }
  err = espconn_regist_disconcb(telnetServer,  telnetDisconectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Telnet registering disconnect cb: %d\r\n", err);
    return err;
  }
  err = espconn_regist_reconcb(telnetServer,   telnetReconnectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Telnet registering error cb: %d\r\n", err);
    return err;
  }
  err = espconn_accept(telnetServer);
  if (err != 0)
  {
    os_printf("\tERROR: Telnet accept: %d\r\n", err);
    return err;
  }

  os_printf("\tNo error\r\n");

  return 0;
}

inline bool ICACHE_FLASH_ATTR telnetIsConnected(void)
{
  return telnetConnection == NULL ? false : true;
}

bool ICACHE_FLASH_ATTR telnetPrintf(const char *format, ...)
{
  va_list argptr;
  uint16 len = 0;
  int8   err = 0;
  char* buffer = (char*)os_zalloc(TELNET_MAX_PRINT_LENGTH);
  if (buffer == NULL)
  {
    os_printf("Couldn't allocate memory for telnet print buffer\r\n");
    return false;
  }
  va_start(argptr, format);
  len = ets_vsnprintf(buffer, TELNET_MAX_PRINT_LENGTH-1, format, argptr);
  va_end(argptr);
  os_printf_plus(buffer);
  if (telnetIsConnected()) // If we have a client
  {
    err = espconn_sent(telnetConnection, (uint8*)buffer, len);
  }
  os_free(buffer);
  if (err < 0)
  {
    os_printf("\tError sending telnet message: %d\r\n", err);
    return false;
  }
  else
  {
    return true;
  }
}
