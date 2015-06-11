/** @file Upgrade / flash controller
 * This module is responsible for reflashing the Espressif
 * @author Daniel Casner <daniel@anki.com>
 */

#include "user_interface.h"
#include "ets_sys.h"
#include "osapi.h"
#include "mem.h"
#include "espconn.h"
#include "upgrade.h"
#include "upgrade_controller.h"

/// Upgrade controller command server
static struct espconn* upccServer;
/// TCP socket for making web server requests
static struct espconn* upClient;
/// Parameters for current upgrade process
static UpgradeCommandParameters* upgradeParameters;
/// Espressif firmware upgrade process structure
static struct upgrade_server_info* espUpgradeServer;

#define UPGRADE_COMMAND_LENGTH (UPGRADE_COMMAND_PREFIX_LENGTH + sizeof(UpgradeCommandParameters))

LOCAL void ICACHE_FLASH_ATTR espressifUpgradeResponseCallback(void *arg)
{
  struct upgrade_server_info *server = arg;
  struct espconn *conn = server->pespconn;
  int8 err;

  if (server->upgrade_flag == true)
  {
    os_printf("Espressif upgrade successful\r\n");
    system_upgrade_reboot();
  }
  else
  {
    os_printf("Espressif upgrade FAILED\r\n");
  }

  os_free(espUpgradeServer);
  espUpgradeServer = NULL;

  if (upClient == NULL)
  {
    os_printf("ERROR: upClient is NULL!\r\n");
  }
  else
  {
    err = espconn_disconnect(upClient);
    if (err != 0)
    {
      os_printf("ERROR: couldn't disconnect upClient: %d\r\n", err);
      os_free(upClient);
      upClient = NULL;
    }
  }
}

LOCAL void ICACHE_FLASH_ATTR upClientReceiveCallback(void *arg, char *usrdata, unsigned short length)
{
    os_printf("upClient recv %d[%d]\r\n", usrdata[0], length);
}

LOCAL void ICACHE_FLASH_ATTR upClientSentCallback(void *arg)
{
  struct espconn *conn = arg;
  os_printf("upClient sent cb\r\n");
}

LOCAL void ICACHE_FLASH_ATTR upClientConnectCallback(void *arg)
{
  static const char* USER1BIN = "user1.bin";
  static const char* USER2BIN = "user2.bin";
  struct espconn *conn = arg;
  int8 err;

  os_printf("Upgrade client connected\r\n");
  espconn_regist_recvcb(conn, upClientReceiveCallback);
  espconn_regist_sentcb(conn, upClientSentCallback);
  // Make the request to the server
  switch (upgradeParameters->command)
  {
    case EspressifFirmwareUpgradeA:
    {
      espUpgradeServer = (struct upgrade_server_info*)os_zalloc(sizeof(struct upgrade_server_info));
      if (espUpgradeServer == NULL)
      {
        os_printf("Couldn't allocate memory for Espressif upgrade server info\r\n");
      }
      else
      {
        os_memset(espUpgradeServer, 0, sizeof(struct upgrade_server_info));
        espUpgradeServer->pespconn = conn;
        os_memcpy(espUpgradeServer->ip, upgradeParameters->serverIP, 4);
        espUpgradeServer->port = upgradeParameters->serverPort;
        espUpgradeServer->check_cb = espressifUpgradeResponseCallback;
        espUpgradeServer->check_times = 120000;
        if (espUpgradeServer->url == NULL)
        {
          espUpgradeServer->url = (uint8*)os_zalloc(128);
          if (espUpgradeServer->url == NULL)
          {
            os_printf("Couldn't allocate memory for espressif upgrade server URL string\r\n");
            break;
          }
        }
        os_sprintf(espUpgradeServer->url, "GET /%s\r\n\r\n", (system_upgrade_userbin_check() == UPGRADE_FW_BIN1 ? USER1BIN : USER2BIN));
        if (system_upgrade_start(espUpgradeServer) == false)
        {
          os_printf("Error starting espressif upgrade\r\n");
        }
        return; // Don't disconnect
      }
      break;
    }
    default:
    {
      os_printf("Upgrade command %d not currently supported\r\n", upgradeParameters->command);
    }
  }
  // Disconnect by default
  err = espconn_disconnect(conn);
  if (err != 0)
  {
    os_printf("\tError disconnecting upClient socket: %d\r\n", err);
  }
}

/// Called after successful disconnect from host
LOCAL void ICACHE_FLASH_ATTR upClientDisconectCallback(void *arg)
{
  os_printf("upClient disconnected\r\n");
  upClient->proto.tcp->local_port = espconn_port();
  if (upgradeParameters->command == EspressifFirmwareUpgradeA && espUpgradeServer == NULL && upClient != NULL)
  {
    os_free(upClient);
    upClient = NULL;
  }
  upgradeParameters->command = UpgradeCommandNone;
}

/// "Reconnect callback is really generic socket error callback"
LOCAL void ICACHE_FLASH_ATTR upClientReconnectCallback(void *arg, sint8 err)
{
  os_printf("upClient error: %d\r\n", err);
  upClientDisconectCallback(arg);
}


/// Callback method for command packets received
static void ICACHE_FLASH_ATTR upgradeControllerCommandCallback(void *arg, char *usrdata, unsigned short len)
{
  if (arg != upccServer)
  {
    os_printf("upgradeControllerCommandCallback called with unexpected argument %08x != %08x\r\n", arg, upccServer);
    return;
  }
  else if (len != UPGRADE_COMMAND_LENGTH)
  {
    os_printf("upgradeControllerCommandCallback got wrong size packet %d[%d] expected [%d]\r\n", usrdata[0], len, UPGRADE_COMMAND_LENGTH);
    return;
  }
  else if (strncmp(UPGRADE_COMMAND_PREFIX, usrdata, UPGRADE_COMMAND_PREFIX_LENGTH) != 0)
  {
    os_printf("upgradeControllerCommandCallback got unexpected packet %d[%d]\r\n", usrdata[0], len);
    return;
  }
  else
  {
    int8 err;
    os_printf("Received upgrade command\r\n");

    if (upgradeParameters == NULL)
    {
      upgradeParameters = (UpgradeCommandParameters*)os_zalloc(sizeof(UpgradeCommandParameters));
      if (upgradeParameters == NULL)
      {
        os_printf("Couldn't allocate memory for upgrade parameters\r\n");
        return;
      }
    }
    os_memcpy(upgradeParameters, (UpgradeCommandParameters*)(usrdata + UPGRADE_COMMAND_PREFIX_LENGTH), sizeof(UpgradeCommandParameters));

    if (upClient == NULL)
    {
      upClient = (struct espconn*)os_zalloc(sizeof(struct espconn));
      if (upClient == NULL)
      {
        os_printf("Couldn't allocate memory for upgradeController client socket\r\n");
        return;
      }
      os_memset(upClient, 0, sizeof(struct espconn));
      upClient->state = ESPCONN_NONE;
      upClient->type  = ESPCONN_TCP;
      upClient->proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
      if (upClient->proto.tcp == NULL)
      {
        os_printf("Coulnt' allocate memory for upgrade controller client tcp parameters\r\n");
        return;
      }
      upClient->proto.tcp->local_port = espconn_port();
      err = espconn_regist_connectcb(upClient, upClientConnectCallback);
      if (err != 0)
      {
        os_printf("Error registering upClient connect cb: %d\r\n", err);
        return;
      }
      err = espconn_regist_disconcb(upClient,  upClientDisconectCallback);
      if (err != 0)
      {
        os_printf("Error registering upClient disconnect cb: %d\r\n", err);
        return;
      }
      err = espconn_regist_reconcb(upClient,   upClientReconnectCallback);
      if (err != 0)
      {
        os_printf("Error registering upClient error cb: %d\r\n", err);
        return;
      }
    }

    os_memcpy(upClient->proto.tcp->remote_ip, upgradeParameters->serverIP, 4);
    upClient->proto.tcp->remote_port = upgradeParameters->serverPort;
    err = espconn_connect(upClient);
    if (err != 0)
    {
      os_printf("Error connecting upClient: %d\r\n", err);
      return;
    }
  }
}

int8_t ICACHE_FLASH_ATTR upgradeControllerInit(void)
{
  int8_t err;

  os_printf("Upgrade controller init\r\n");

  upClient = NULL;
  upgradeParameters = NULL;
  espUpgradeServer = NULL;

  upccServer = (struct espconn*)os_zalloc(sizeof(struct espconn));
  if (upccServer == NULL)
  {
    os_printf("\tCouldn't allocate memory for upgradeController command socket\r\n");
    return -100;
  }
  os_memset(upccServer, 0, sizeof(struct espconn));
  upccServer->state = ESPCONN_NONE;
  upccServer->type  = ESPCONN_UDP;
  upccServer->proto.udp = (esp_udp*)os_zalloc(sizeof(esp_udp));
  if (upccServer->proto.udp == NULL)
  {
    os_printf("\tCouldn't allocate memory for upgradeController command socket UDP parameters\r\n");
    return -101;
  }
  upccServer->proto.udp->local_port = 8580;

  err = espconn_regist_recvcb(upccServer, upgradeControllerCommandCallback);
  if (err != 0)
  {
    os_printf("\tError registerring receive callback for upgradeController command socket: %d\r\n", err);
    return err;
  }
  err = espconn_create(upccServer);
  if (err != 0)
  {
    os_printf("\tError creating upgradeController command socket: %d\r\n", err);
    return err;
  }

  os_printf("\tNo error\r\n");

  return 0;
}
