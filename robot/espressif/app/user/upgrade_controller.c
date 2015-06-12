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

/// Upgrade controller server
static struct espconn* upccServer;

static uint32 fwWriteAddress = 0;
static uint32 bytesExpected  = 0;
static uint32 bytesReceived  = 0;
static uint8  flags          = 0;


/// Resets the upgrade state
LOCAL void ICACHE_FLASH_ATTR resetUpgradeState(void)
{
  system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
  fwWriteAddress = INVALID_FLASH_ADDRESS;
  bytesExpected  = 0;
  bytesReceived  = 0;
  flags = UPCMD_FLAGS_NONE;
}

LOCAL void ICACHE_FLASH_ATTR printUpgradeState(void)
{
  os_printf("Upgrade state: fww = %08x, bytesE = %d, bytesR = %d, flags = %d, esp = %d\r\n", fwWriteAddress, bytesExpected, bytesReceived, flags, system_upgrade_flag_check());
}


LOCAL void ICACHE_FLASH_ATTR upccReceiveCallback(void *arg, char *usrdata, unsigned short length)
{
  struct espconn* conn = (struct esponn*)arg;
  uint8 err;

  os_printf("INFO: UPCC recv %d[%d]\r\n", usrdata[0], length);
  if (fwWriteAddress == INVALID_FLASH_ADDRESS) // Start of new command
  {
    if (length != sizeof(UpgradeCommandParameters))
    {
      os_pritnf("ERROR: UPCC received wrong command packet size %d <> %d\r\n", length, sizeof(UpgradeCommandParameters));
      err = espconn_sent(conn, "BAD", 3);
      if (err != 0)
      {
        os_printf("ERROR: UPCC couldn't sent error response on socket: %d\r\n", err);
      }
      return;
    }
    UpgradeCommandParameters* cmd = (UpgradeCommandParameters*)usrdata;
    if (strncmp(cmd->PREFIX, UPGRADE_COMMAND_PREFIX, UPGRADE_COMMAND_PREFIX_LENGTH) != 0) // Wrong PREFIX
    {
      os_printf("ERROR: UPCC received invalid header\r\n");
    }
    else
    {
      fwWriteAddress = cmd->flashAddress;
      bytesExpected  = cmd->size;
      flags          = cmd->flags;
      if (flags & UPCMD_WIFI_FW)
      {
        uint8 response[4];
        response[0] = 'O';
        response[1] = 'K';
        response[2] = flags;
        response[4] system_upgrade_userbin_check();
        err = espconn_sent(conn, response, 4);
        if (err != 0)
        {
          os_printf("ERROR: UPCC sending command response: %d\r\n", err);
          resetUpgradeState();
          return;
        }
        system_upgrade_flag_set(UPGRADE_FLAG_START);

      }
    }
  }
}

LOCAL void ICACHE_FLASH_ATTR upccSentCallback(void *arg)
{
  struct espconn *conn = arg;
  os_printf("INFO: UPCC sent cb\r\n");
}

LOCAL void ICACHE_FLASH_ATTR upClientConnectCallback(void *arg)
{
  struct espconn* conn = (struct espconn*)arg;
  uint8 err;

  os_printf("INFO: UPCC Connected\r\n");

  if (fwWriteAddress != INVALID_FLASH_ADDRESS)
  {
    os_printf("ERROR: Got new upgrade connection while not idle\r\n\t");
    printUpgradeState();
    return;
  }

  err = espconn_regist_recvcb(conn, upccReceiveCallback);
  if (err != 0)
  {
    os_printf("ERROR: UPCC couldn't register receive callback: %d\r\n", err);
    return;
  }
  err = espconn_regist_sentcb(conn, upccSentCallback);
  if (err != 0)
  {
    os_printf("ERROR: UPCC couldn't register sent callback: %d\r\n", err);
    return;
  }
  resetUpgradeState();
}

/// Called after successful disconnect from host
LOCAL void ICACHE_FLASH_ATTR upccDisconectCallback(void *arg)
{
  os_printf("INFO: UPCC disconnected\r\n");

  if (flags & UPCMD_WIFI_FW)
  {
    const uint8 espUpgradeState = system_upgrade_flag_check();
    if (espUpgradeState != UPGRADE_FLAG_START)
    {
      os_printf("ERROR: end of connection with WiFi flag wasn't in right state\r\n");
      resetUpgradeState();
    }
    else if(bytesReceived != bytesExpected)
    {
      os_printf("ERROR: Upgrade connection closed with with wrong amount of data: %d <> %d\r\n", bytesReceived, bytesExpected);
      resetUpgradeState();
    }
    else
    {
      os_printf("INFO: Sytem upgrade finished, rebooting\r\n");
      system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
      system_upgrade_reboot();
    }
  }
  else if (flags != UPCMD_FLAGS_NONE)
  {
    os_printf("Unhandled upgrade flags: %02x\r\n", flags);
  }
}

/// "Reconnect callback is really generic socket error callback"
LOCAL void ICACHE_FLASH_ATTR upccReconnectCallback(void *arg, sint8 err)
{
  os_printf("ERROR UPCC error %d\r\n\t", err);
  printUpgradeState();
  resetUpgradeState();
}

int8_t ICACHE_FLASH_ATTR upgradeControllerInit(void)
{
  int8_t err;

  os_printf("Upgrade controller init\r\n");

  resetUpgradeState();

  upccServer = (struct espconn*)os_zalloc(sizeof(struct espconn));
  if (upccServer == NULL)
  {
    os_printf("\tCouldn't allocate memory for upgradeController command socket\r\n");
    return -100;
  }
  os_memset(upccServer, 0, sizeof(struct espconn));
  upccServer->state = ESPCONN_NONE;
  upccServer->type  = ESPCONN_TCP;
  upccServer->proto.tcp = (esp_tcp*)os_zalloc(sizeof(esp_tcp));
  if (upccServer->proto.tcp == NULL)
  {
    os_printf("\tCouldn't allocate memory for upgradeController command socket UDP parameters\r\n");
    return -101;
  }
  upccServer->proto.tcp->local_port = 8580;

  err = espconn_regist_connectcb(upccServer, upccConnectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Upgrade controller registering connect cb: %d\r\n", err);
    return -102;
  }
  err = espconn_regist_disconcb(upccServer,  upccDisconectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Upgrade controller registering disconnect cb: %d\r\n", err);
    return -103;
  }
  err = espconn_regist_reconcb(upccServer,   upccReconnectCallback);
  if (err != 0)
  {
    os_printf("\tERROR: Upgrade controller registering upClient error cb: %d\r\n", err);
    return -104;
  }
  err = espconn_accept(upccServer);
  if (err != 0)
  {
    os_printf("\tERROR: Upgrade controller accept: %d\r\n", err)l
    return -105;
  }

  os_printf("\tNo error\r\n");

  return 0;
}
