#include "mem.h"
#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "client.h"
#include "driver/uart.h"

#define min(a, b) (a < b ? a : b)

#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

#define rfrxbuf_len 2048 // Must be a power of 2
#define rfrxbuf_lenmask (rfrxbuf_len - 1)
static uint16 rfrxWind = 0;
static uint16 rfrxRind = 0;
static uint8 uartTXBuf[3200];





void ICACHE_FLASH_ATTR client_receiveCB(uint8* data, uint16 len)
{
  uint16 available = rfrxbuf_len - ((rfrxWind - rfrxRind) & rfrxbuf_lenmask);
  uint16 writeOne;

  if ((len + 6) > available)
  {
    os_printf("RFRX, no space %d > %d\r\n", len + 6, available);
    return;
  }

  uartTXBuf[rfrxWind] = 0xbe; rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;
  uartTXBuf[rfrxWind] = 0xef; rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;
  uartTXBuf[rfrxWind] = (len & 0xff); rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;
  uartTXBuf[rfrxWind] = ((len >> 8) & 0xff); rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;
  uartTXBuf[rfrxWind] = 0x00; rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;
  uartTXBuf[rfrxWind] = 0x00; rfrxWind = (rfrxWind + 1) & rfrxbuf_lenmask;

  writeOne = min(len, rfrxbuf_len - rfrxWind);
  os_memcpy(uartTXBuf + rfrxWind, data, writeOne);
  if (writeOne < len)
  {
    os_memcpy(uartTXBuf, data + writeOne, len - writeOne);
  }
  rfrxWind = (rfrxWind + len) & rfrxbuf_lenmask;

  system_os_post(user_procTaskPrio, 0, 0 );
}


static void ICACHE_FLASH_ATTR uartTxTask(os_event_t *events)
{
  while (rfrxRind != rfrxWind)
  {
    uart_tx_one_char(uartTXBuf[rfrxRind]);
    rfrxRind = (rfrxRind + 1) & rfrxbuf_lenmask;
  }
}

inline void uart0_recvCB()
{
  static uint32 serRxLen = 0;
  static UDPPacket* outPkt = NULL;
  static uint8 phase = 0;
  uint8 byte = 0;
  uint8 err;

  while (READ_PERI_REG(UART_STATUS(0)) & (UART_RXFIFO_CNT << UART_RXFIFO_CNT_S))
  {
    byte = READ_PERI_REG(UART_FIFO(0)) & 0xFF;

    switch (phase)
    {
      case 0: // Header byte 1
      {
        if (byte == 0xbe) phase++;
        break;
      }
      case 1: // Header byte 2
      {
        if (byte == 0xef) phase++;
        else phase = 0;
        break;
      }
      case 2: // Packet length, 4 bytes
      {
        serRxLen  = (byte <<  0);
        phase++;
        break;
      }
      case 3:
      {
        serRxLen |= (byte <<  8);
        phase++;
        break;
      }
      case 4:
      {
        serRxLen |= (byte << 16);
        phase++;
        break;
      }
      case 5:
      {
        serRxLen |= (byte << 24);
        if (serRxLen < PKT_BUFFER_SIZE) phase++;
        else phase = 0; // Got an invalid length, have to throw out the packet
        break;
      }
      case 6: // Start of payload
      {
        outPkt = clientGetBuffer();
        if (outPkt == NULL)
        {
          phase = 0;
          break;
        }
        else
        {
          outPkt->len = 0;
          phase++;
        }
        // No break, explicit fall through to next case
      }
      case 7:
      {
        outPkt->data[outPkt->len++] = byte;
        if (outPkt->len >= serRxLen)
        {
          clientQueuePacket(outPkt);
          phase = 0;
        }
        break;
      }
      default:
      {
        os_printf("UART phase default, SHOULD NEVER HAPPEN!\r\n");
        phase = 0;
      }
    }
  }
}


//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    uint32 i;
    int8 err;

    REG_SET_BIT(0x3ff00014, BIT(0));
    os_update_cpu_frequency(160);

    //uart_div_modify(0, UART_CLK_FREQ / 3000000);

    uart_init(BIT_RATE_3000000, BIT_RATE_115200);

    os_printf("Espressif booting up...\r\n");

    // Create config for Wifi AP
    struct softap_config ap_config;

    os_strcpy(ap_config.ssid, "AnkiEspressif");

    os_strcpy(ap_config.password, "2manysecrets");
    ap_config.ssid_len = 0;
    ap_config.channel = 7;
    ap_config.authmode = AUTH_WPA2_PSK;
    ap_config.max_connection = 4;

    // Setup ESP module to AP mode and apply settings
    wifi_set_opmode(SOFTAP_MODE);
    wifi_softap_set_config(&ap_config);
    wifi_set_phy_mode(PHY_MODE_11G);

    // Create ip config
    struct ip_info ipinfo;
    ipinfo.gw.addr = ipaddr_addr("172.31.1.1");
    ipinfo.ip.addr = ipaddr_addr("172.31.1.1");
    ipinfo.netmask.addr = ipaddr_addr("255.255.255.0");

    // Assign ip config
    wifi_set_ip_info(SOFTAP_IF, &ipinfo);

    // Start DHCP server
    wifi_softap_dhcps_start();

    // Setup Basestation client
    clientInit(client_receiveCB);

    // Setup the
    system_os_task(uartTxTask, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);

    os_printf("user initalization complete\r\n");

}
