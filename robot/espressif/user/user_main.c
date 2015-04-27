#include "mem.h"
#include "c_types.h"
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "client.h"
#include "driver/uart.h"

/// USER_TASK_PRIO_0 is the lowest (idle) task priority
#define userTaskPrio USER_TASK_PRIO_0

/// System task
#define    userTaskQueueLen    4
os_event_t userTaskQueue[userTaskQueueLen];

/** User "idle" task
 * Called by OS with lowest priority.
 * Always requeues itself.
 */
LOCAL void ICACHE_FLASH_ATTR userTask(os_event_t *event)
{
  uart_tx_one_char_no_wait(UART1, '.'); // Print a dot to show we're executing
  system_os_post(userTaskPrio, 0, 0); // Repost ourselves
}

/** User initialization function
 * This function is responsible for setting all the wireless parameters and
 * Setting up any user application code to run on the espressif.
 * It is called automatically from the os main function.
 */
void ICACHE_FLASH_ATTR
user_init()
{
    uint32 i;
    int8 err;

    REG_SET_BIT(0x3ff00014, BIT(0));
    os_update_cpu_frequency(160);

    uart_init(BIT_RATE_3686400, BIT_RATE_74880);

    os_printf("Espressif booting up...\r\n");

    // Create config for Wifi AP
    struct softap_config ap_config;

    os_strcpy(ap_config.ssid, "AnkiEspressif");

    os_strcpy(ap_config.password, "2manysecrets");
    ap_config.ssid_len = 0;
    ap_config.channel = 2;
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
    clientInit();

    // Setup user task
    system_os_task(userTask, userTaskPrio, userTaskQueue, userTaskQueueLen); // Initalize OS task
    system_os_post(userTaskPrio, 0, 0); // Post user task

    os_printf("user initalization complete\r\n");

}
