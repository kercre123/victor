A Node.js app to talk to Victors over BLE

## Installation

```sh
$ ./install.sh
```

## Configuring WiFi

First, power on the victor and wait about 1 minute to get it to start advertising via BLE.  Then


```sh
$ node ./index.js
$ scan

Scanning started.....
$
Found VICTOR_553261a
$ connect

Trying to connect to VICTOR_553261a....
$
Connected to VICTOR_553261a
$
Services discovered for VICTOR_553261a
$
Fully connected to VICTOR_553261a
$ wpa_cli list_networks
$
Using interface 'wlan0'
network id / ssid / bssid / flags

$ wpa_cli add_network
$
Using interface 'wlan0'
0

$ wpa_cli set_network 0 ssid "NETWORKNAME"
$
Using interface 'wlan0'
OK

$ wpa_cli set_network 0 psk "PASSWORD"
$
Using interface 'wlan0'
OK

$ wpa_cli enable_network 0
$
Using interface 'wlan0'
OK

$ wpa_cli save_config
$
Using interface 'wlan0'
OK

$ dhcptool wlan0
$

$ ifconfig wlan0
$
wlan0     Link encap:Ethernet  HWaddr 00:0a:f5:4c:98:68
          inet addr:192.168.77.70  Bcast:192.168.77.255  Mask:255.255.255.0
          inet6 addr: fe80::20a:f5ff:fe4c:9868/64 Scope: Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:296 errors:0 dropped:0 overruns:0 frame:0
          TX packets:12 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:72798 TX bytes:1416
```
