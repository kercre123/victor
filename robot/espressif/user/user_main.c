#include "mem.h"
#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "espconn.h"
#include "driver/uart.h"


#define user_procTaskPrio        0
#define user_procTaskQueueLen    1
os_event_t    user_procTaskQueue[user_procTaskQueueLen];

static volatile os_timer_t userTimer;
static struct espconn *pUdpServer;
struct espconn *pespconn = NULL;

#define NUM_RTX_BUFS 4
#define MTU 1500

typedef struct
{
  uint16 len;
  uint8 data[1500];
  bool queued;
} RadioTXBuffer;

static RadioTXBuffer rtxbs[NUM_RTX_BUFS];

#define user_print(message) uart0_tx_buffer(message, os_strlen(message))

char dbg_text[120];

//Idle task
static void ICACHE_FLASH_ATTR
loop(os_event_t *events)
{
  system_os_post(user_procTaskPrio, 0, 0 );
}

// Interval Task
static void ICACHE_FLASH_ATTR
interval(void)
{

}

inline void uart0_recvCB()
{
  static uint32 serRxLen = 0;
  static uint8 phase = 0;
  static uint8 which = 0;
  uint8 byte = 0;
  uint8 i, err;

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
        if (serRxLen < MTU) phase++;
        else phase = 0; // Got an invalid length, have to throw out the packet
        break;
      }
      case 6: // Start of payload
      {
        // Check if we have somewhere to send this data
        if (pespconn == NULL) { // Nope
          phase = 0;
          user_print("NCDP\r\n");
          break;
        }
        else
        {
          if (rtxbs[which].queued)
          {
            user_print("NBADP\r\n");
            phase = 0;
            break;
          }
          else
          {
            rtxbs[which].len = 0;
            phase++;
          }
        }
        // No break, explicit fall through to next case
      }
      case 7:
      {
        rtxbs[which].data[rtxbs[which].len++] = byte;
        if (rtxbs[which].len >= serRxLen)
        {
          err = espconn_sent(pespconn, rtxbs[which].data, rtxbs[which].len);
          if (err == ESPCONN_OK)
          {
            rtxbs[which].queued = true;
            which = (which + 1) % NUM_RTX_BUFS;
          }
          else
          {
            os_sprintf(dbg_text, "espconn err %d\r\n", err);
          }
          phase = 0;
        }
        break;
      }
      default:
      {
        user_print("UART phase default, SHOULD NEVER HAPPEN!\r\n");
        phase = 0;
      }
    }
  }
}

static void udpserver_sent_cb(void* arg)
{
  static u8 which = 0;

  rtxbs[which].queued = false;
  which = (which + 1) % NUM_RTX_BUFS;

  // Else handle error
}

//Called when new packet comes in.
static void udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
  char err = 0;
  pespconn = (struct espconn *)arg;

  user_print("udp recv CB\r\n");

  espconn_regist_sentcb(pespconn, udpserver_sent_cb);
}

//Init function
void ICACHE_FLASH_ATTR
user_init()
{
    uint32 i;
    int8 err;

    REG_SET_BIT(0x3ff00014, BIT(0));
    os_update_cpu_frequency(160);

    uart_div_modify(0, UART_CLK_FREQ / 3000000);

    for (i=0; i<NUM_RTX_BUFS; ++i)
    {
      rtxbs[i].len = 0;
      rtxbs[i].queued = false;
    }

    //uart_div_modify(0, UART_CLK_FREQ / 3000000);
    uart_init(BIT_RATE_3000000, BIT_RATE_9600);

    os_delay_us(1000000);

    user_print("\r\nBooting up...\r\n");

    // Create config for Wifi AP
    struct softap_config ap_config;

    os_strcpy(ap_config.ssid, "AnkiEspressif");

    os_strcpy(ap_config.password, "2manysecrets");
    ap_config.ssid_len = 0;
    ap_config.channel = 7;
    ap_config.authmode = AUTH_WPA2_PSK;
    ap_config.max_connection = 4;

    // Setup ESP module to AP mode and apply settings
    if (wifi_set_opmode(SOFTAP_MODE)) {
      user_print("Successfully set up OPMODE\r\n");
    }
    else {
      user_print("FAILED to set OPMODE\r\n");
    }
    if (wifi_softap_set_config(&ap_config)) {
      user_print("Successfully set softap config\r\n");
    }
    else {
      user_print("FAILED to set softap config\r\n");
    }
    if (wifi_set_phy_mode(PHY_MODE_11G)) {
      user_print("Successfully set phy mode\r\n");
    }
    else {
      user_print("FAILED to set phy mode\r\n");
    }

    // Create ip config
    struct ip_info ipinfo;
    ipinfo.gw.addr = ipaddr_addr("172.31.1.1");
    ipinfo.ip.addr = ipaddr_addr("172.31.1.1");
    ipinfo.netmask.addr = ipaddr_addr("255.255.255.0");

    // Assign ip config
    if (wifi_set_ip_info(SOFTAP_IF, &ipinfo)) {
      user_print("Successfully set IP info\r\n");
    }
    else {
      user_print("FAILED to set IP info\r\n");
    }

    // Start DHCP server
    if (wifi_softap_dhcps_start()) {
      user_print("Successfully started DHCP server\r\n");
    }
    else {
      user_print("FAILED to start DHCP server\r\n");
    }

    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
    ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
    espconn_create( pUdpServer );
    pUdpServer->type = ESPCONN_UDP;
    pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
    pUdpServer->proto.udp->local_port = 6661;
    espconn_regist_recvcb(pUdpServer, udpserver_recv);

    wifi_station_dhcpc_start();

    err = espconn_create( pUdpServer );
    if (err) {
      user_print("FAILED to create UDP server\r\n");
    }
    else {
      user_print("Successfully created UDP server\r\n");
    }

    // Start Timer
    //os_timer_disarm(&userTimer);
    //os_timer_setfn(&userTimer, (os_timer_func_t*)interval, NULL);
    //os_timer_arm(&userTimer, 1, 1);

    //Start os task
    //system_os_task(loop, user_procTaskPrio,user_procTaskQueue, user_procTaskQueueLen);
    //system_os_post(user_procTaskPrio, 0, 0 );
}
