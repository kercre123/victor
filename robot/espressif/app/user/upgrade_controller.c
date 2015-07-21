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
#include "task0.h"
#include "driver/spi.h"
#include "fpga.h"
#include "upgrade_controller.h"

/// Flash sector size for erasing, 4KB
#define FLASH_SECTOR_SIZE 4096
/// Size of chunks we expect to receive for writing, 1KB
#define FLASH_WRITE_SIZE 1024

#define MAX_RETRIES 2

/// Stuct for holding incoming data to write to flash
typedef struct {
  uint32 data[FLASH_WRITE_SIZE/4]; // 4 byte aligned data to write
  uint32 length; // Number of BYTES to write
} FlashWriteData;


/// Upgrade controller server
static struct espconn* upccServer;
/// Current connection for upgrade transaction
static struct espconn* upccConn;

static uint32 fwWriteAddress = 0;
static uint32 bytesExpected  = 0;
static uint32 bytesReceived  = 0;
static uint32 bytesWritten   = 0;
static uint32 flags          = 0;

typedef enum {
  UPGRADE_TASK_IDLE,
  UPGRADE_TASK_ERASE,
  UPGRADE_TASK_WRITE,
  UPGRADE_TASK_FINISH,
  UPGRADE_TASK_FPGA_RESET,
  UPGRADE_TASK_FPGA_WRITE,
} UpgradeTaskState;

static UpgradeTaskState taskState = UPGRADE_TASK_IDLE;
static uint16 flashEraseSector = 0;
static uint16 flashEraseEnd    = 0;
static uint16 retryCount = 0;

/// Resets the upgrade state
LOCAL void ICACHE_FLASH_ATTR resetUpgradeState(void)
{
  system_upgrade_flag_set(UPGRADE_FLAG_IDLE);
  fwWriteAddress = INVALID_FLASH_ADDRESS;
  bytesExpected  = 0;
  bytesReceived  = 0;
  bytesWritten   = 0;
  flags          = UPCMD_FLAGS_NONE;
  taskState      = UPGRADE_TASK_IDLE;
  flashEraseSector = 0;
  flashEraseEnd    = 0;
  retryCount       = 0;
  upccConn = NULL;
}

LOCAL void ICACHE_FLASH_ATTR printUpgradeState(void)
{
  os_printf("Upgrade state: fww = %08x, bytesE = %d, bytesR = %d, bytesW = %d, flags = %d, ts= %d, esp = %d\r\n", fwWriteAddress, bytesExpected, bytesReceived, bytesWritten, flags, taskState, system_upgrade_flag_check());
}

LOCAL inline void requestNext(void)
{
  if (upccConn != NULL)
  {
    int8_t err = espconn_sent(upccConn, "next", 4);
    if (err != 0)
    {
      os_printf("\tCouldn't unhold socket: %d\r\n", err);
    }
  }
  else
  {
    os_printf("ERROR: upccConn NULL\r\n");
    resetUpgradeState();
  }
}

LOCAL bool upgradeTask(uint32_t param)
{
  SpiFlashOpResult flashResult;
  int8 err;

  switch (taskState)
  {
    case UPGRADE_TASK_IDLE:
    {
      os_printf("WARN: upgradeTask called with taskState idle\r\n\t");
      printUpgradeState();
      return false;
    }
    case UPGRADE_TASK_ERASE:
    {
      os_printf("UP: Erase %d..%d\r\n", flashEraseSector, flashEraseEnd);
      flashResult = spi_flash_erase_sector(flashEraseSector);
      switch (flashResult)
      {
        case SPI_FLASH_RESULT_OK:
        {
          retryCount = 0;
          if (flashEraseSector == flashEraseEnd)
          {
            os_printf("UP: Erase complete\r\n");
            taskState = UPGRADE_TASK_WRITE;
            requestNext();
            return false;
          }
          else
          {
            flashEraseSector++;
            return true;
          }
        }
        case SPI_FLASH_RESULT_ERR:
        {
          os_printf("\tError erasing sector\r\n");
          resetUpgradeState();
          return false;
        }
        case SPI_FLASH_RESULT_TIMEOUT:
        {
          if (retryCount++ < MAX_RETRIES)
          {
            os_printf("\tflash timeout, retrying\r\n");
            return true;
          }
          else
          {
            os_printf("\tflash timeout, aborting\r\n");
            resetUpgradeState();
            return false;
          }
        }
        default:
        {
          os_printf("\tUnhandled flash result: %d\r\n", flashResult);
          resetUpgradeState();
          return false;
        }
      }
    }
    case UPGRADE_TASK_WRITE:
    {
      if (fwWriteAddress < FW_START_ADDRESS)
      {
        os_printf("UP ERROR: Won't write to %08x below %08x\r\n", fwWriteAddress, FW_START_ADDRESS);
        resetUpgradeState();
        return false;
      }
      else if (param == 0)
      {
        os_printf("UP ERROR: UPGRADE TASK WRITE but param = 0\r\n");
        resetUpgradeState();
        return false;
      }
      FlashWriteData* fwd = (FlashWriteData*)param;
      os_printf("UP: Write 0x%08x 0x%x\r\n", fwWriteAddress + bytesWritten, fwd->length);
      flashResult = spi_flash_write(fwWriteAddress + bytesWritten, fwd->data, fwd->length);
      switch (flashResult)
      {
        case SPI_FLASH_RESULT_OK:
        {
          retryCount = 0;
          bytesWritten += fwd->length;
          os_free(fwd); // Free the memory used
          if (bytesWritten == bytesExpected)
          {
            taskState = UPGRADE_TASK_FINISH;
            return true;
          }
          else
          {
            requestNext();
            return false;
          }
        }
        case SPI_FLASH_RESULT_ERR:
        {
          os_printf("\twrite error\r\n");
          os_free(fwd);
          resetUpgradeState();
          return false;
        }
        case SPI_FLASH_RESULT_TIMEOUT:
        {
          if (retryCount++ < MAX_RETRIES)
          {
            os_printf("\twrite timeout\r\n");
            return true;
          }
          else
          {
            os_printf("\twrite timout, aborting\r\n");
            os_free(fwd);
            resetUpgradeState();
            return false;
          }
        }
        default:
        {
          os_printf("\tUnhandled flash result: %d\r\n", flashResult);
          os_free(fwd);
          resetUpgradeState();
          return false;
        }
      }
    }
    case UPGRADE_TASK_FINISH:
    {
      if (flags & UPCMD_WIFI_FW)
      {
        // TODO check new firmware integrity
        system_upgrade_flag_set(UPGRADE_FLAG_FINISH);
        system_upgrade_reboot();
      }
      if (flags & UPCMD_CTRL_FW)
      {
        // TODO check new firmware integrity
        // TODO restart with new firmware
      }
      if (flags & UPCMD_FPGA_FW)
      {
        // TODO check new firmware integrity
        // TODO Reboot the FPGA with new firmware
        os_printf("UP: FPGA upgrade complete\r\n\t");
        printUpgradeState();
        resetUpgradeState();
      }
      if (flags & UPCMD_BODY_FW)
      {
        // TODO check new firmware integrity
        // TODO Reboot the body with new firmware
      }
      return false;
    }
    case UPGRADE_TASK_FPGA_RESET:
    {
      if (flags & UPCMD_FPGA_FW)
      {
        fpgaDisable();
        os_delay_us(2);
        fpgaEnable();
        os_delay_us(300);
        taskState = UPGRADE_TASK_FPGA_WRITE;
        requestNext();
        return false;
      }
      else
      {
        os_printf("ERR: upgradeTask state RESET FPGA but flags don't say FPGA?\r\n");
        printUpgradeState();
        resetUpgradeState();
        return false;
      }
    }
    case UPGRADE_TASK_FPGA_WRITE:
    {
      if (param == 0)
      {
        os_printf("UP ERROR: UPGRADE TASK FPGA WRITE but param = 0\r\n");
        resetUpgradeState();
        return false;
      }
      if (flags & UPCMD_FPGA_FW)
      {
        uint32_t spiWritten = 0;
        FlashWriteData* fwd = (FlashWriteData*)param;
        uint32_t wordsToWrite = fwd->length / 4;
        os_printf("UP: FPGA Write 0x%08x 0x%x\r\n", fwWriteAddress + bytesWritten, fwd->length);
        while (spiWritten < wordsToWrite)
        {
          if (!fpgaWriteBitstream((fwd->data + spiWritten)))
          {
            os_printf("UP ERR: Couldn't write bytes to FPGA\r\n");
            resetUpgradeState();
            return false;
          }
          else
          {
            spiWritten += SPI_FIFO_DEPTH;
          }
        }

        os_free(fwd); // Free the memory used
        bytesWritten += fwd->length;
        if (bytesWritten >= bytesExpected)
        {
          taskState = UPGRADE_TASK_FINISH;
          return true;
        }
        else
        {
          requestNext();
          return false;
        }
      }
      else
      {
        os_printf("ERR: upgradeTask state WRITE FPGA but flags don't say FPGA?\r\n");
        printUpgradeState();
        resetUpgradeState();
        return false;
      }
    }
    default:
    {
      os_printf("WARN: upgradeTask unexpected state: %d\r\n\t", taskState);
      printUpgradeState();
      return false;
    }
  }
}

LOCAL void ICACHE_FLASH_ATTR upccReceiveCallback(void *arg, char *usrdata, unsigned short length)
{
  struct espconn* conn = (struct espconn*)arg;
  uint8 err;

  //os_printf("INFO: UPCC recv %d[%d]\r\n", usrdata[0], length);

  if (fwWriteAddress == INVALID_FLASH_ADDRESS) // Start of new command
  {
    if (length != sizeof(UpgradeCommandParameters))
    {
      os_printf("ERROR: UPCC received wrong command packet size %d <> %d\r\n", length, sizeof(UpgradeCommandParameters));
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
    else if (cmd->flashAddress < FW_START_ADDRESS)
    {
      os_printf("ERROR: fwWriteAddress %08x is in protected region\r\n", cmd->flashAddress);
      err = espconn_sent(conn, "BAD", 3);
      if (err != 0)
      {
        os_printf("ERROR: UPCC couldn't sent error response on socket: %d \r\n", err);
      }
      return;
    }
    else
    {
      if (cmd->flags & UPCMD_WIFI_FW)
      {
        uint8 response[4];
        response[0] = 'O';
        response[1] = 'K';
        response[2] = cmd->flags & 0xff;
        response[3] = system_upgrade_userbin_check();
        err = espconn_sent(conn, response, 4);
        if (err != 0)
        {
          os_printf("ERROR: UPCC sending command response: %d\r\n", err);
          resetUpgradeState();
          return;
        }
        if (!(cmd->flags & UPCMD_ASK_WHICH)) // Wasn't just an inquiry
        {
          fwWriteAddress   = cmd->flashAddress;
          bytesExpected    = cmd->size;
          flags            = cmd->flags;
          flashEraseSector = cmd->flashAddress / FLASH_SECTOR_SIZE;
          flashEraseEnd    = (cmd->flashAddress + cmd->size) / FLASH_SECTOR_SIZE;
          upccConn         = conn;
          os_printf("fwWriteAddr = %08x [%x], Erase sectors: %d..%d\r\n", fwWriteAddress, bytesExpected, flashEraseSector, flashEraseEnd);
          system_upgrade_flag_set(UPGRADE_FLAG_START);
          taskState = UPGRADE_TASK_ERASE;
          if (task0Post(upgradeTask, 0) == false)
          {
            os_printf("ERROR: Couldn't queue upgrade task\r\n");
            resetUpgradeState();
            return;
          }
        }
      }
      else if (cmd->flags & UPCMD_FPGA_FW)
      {
        fwWriteAddress   = cmd->flashAddress;
        bytesExpected    = cmd->size;
        flags            = cmd->flags;
        upccConn = conn;
        taskState = UPGRADE_TASK_FPGA_RESET;
        os_printf("FPGA write firmware %d bytes, flags = %x \r\n", bytesExpected, flags);
        if (task0Post(upgradeTask, 0) == false)
        {
          os_printf("ERROR: Couldn't queue upgrade task\r\n");
          resetUpgradeState();
          return;
        }
      }
      else
      {
        os_printf("Unsupported upgrade command flag: %d\r\n", flags);
        err = espconn_sent(conn, "BAD", 3);
        if (err != 0)
        {
          os_printf("ERROR: UPCC couldn't sent error response on socket: %d\r\n", err);
        }
        return;
      }
    }
  }
  else
  {
    FlashWriteData* fwd;
    if (length > FLASH_WRITE_SIZE)
    {
      os_printf("ERROR: UPCC Received too big a chunk of data!\r\n");
      resetUpgradeState();
      return;
    }
    if (length % 4 != 0)
    {
      os_printf("ERROR: UPCC Received non-alignted data %d\r\n", length);
      return;
    }
    fwd = (FlashWriteData*)os_zalloc(sizeof(FlashWriteData));
    if (fwd == NULL)
    {
      os_printf("ERROR: couldn't allocate memory to queue flash write data\r\n");
      return;
    }
    os_memcpy((void*)fwd->data, usrdata, length);
    fwd->length = length;
    bytesReceived += length;
    if (task0Post(upgradeTask, (uint32)fwd) == false)
    {
      os_printf("ERROR: Couldn't post upgrade task\r\n");
      resetUpgradeState();
      os_free(fwd);
    }
  }
}

LOCAL void ICACHE_FLASH_ATTR upccSentCallback(void *arg)
{
  struct espconn *conn = arg;
  //os_printf("INFO: UPCC sent cb\r\n");
}

LOCAL void ICACHE_FLASH_ATTR upccConnectCallback(void *arg)
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
  if (bytesReceived != bytesExpected)
  {
    os_printf("INFO: UPCC disconnected without receiving expected data: %d <> %d\r\n\t", bytesReceived, bytesExpected);
    printUpgradeState();
    resetUpgradeState();
  }
  else
  {
    os_printf("INFO: UPCC disconnected\r\n");
  }
  if (arg == upccConn)
  {
    upccConn = NULL;
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
    os_printf("\tERROR: Upgrade controller accept: %d\r\n", err);
    return -105;
  }

  os_printf("\tNo error\r\n");

  return 0;
}
