/** This file is required by the iot_sdk include osapi.h though it doesn't appear that it actually has to have any
 * contents.
 */


#ifdef STATION_MODE

#define STATION_SSID "AnkiRobits"
#define STATION_KEY  "KlaatuBaradaNikto!"
#define STATION_IP      "192.168.3.30"
#define STATION_NETMASK "255.255.248.0"
#define STATION_GATEWAY "192.168.2.1"

#else

#define AP_SSID_FMT "AnkiEspressif%02x%02x"
#define AP_KEY      "2manysecrets"
#define AP_IP       "172.31.1.1"
#define AP_NETMASK  "255.255.255.0"
#define AP_GATEWAY  "0.0.0.0"
#define DHCP

#endif
