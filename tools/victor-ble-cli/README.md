A Node.js app to talk to Victors over BLE

## Installation

```sh
bash$ ./install.sh
```

## Configuring WiFi

First, power on the victor and wait about 1 minute to get it to start advertising via BLE.  Then


```sh
bash$ node ./index.js
victor-ble-cli$ scan

Please power on your Bluetooth adapter to start scanning.
victor-ble-cli$
Scanning started.....
victor-ble-cli$
Found Vector T7R9 (RSSI = -51)
victor-ble-cli$ connect "Vector T7R9"

Scanning stopped.
victor-ble-cli$
Trying to connect to Vector T7R9....
victor-ble-cli$
Connected to Vector T7R9
victor-ble-cli$
Services discovered for Vector T7R9
victor-ble-cli$
Fully connected to Vector T7R9
victor-ble-cli$ wifi-set-config psk false SSID PASSWORD
victor-ble-cli$
Error wifi: Already enabled

victor-ble-cli$
Give me about 15 seconds to setup WiFi....

victor-ble-cli$
wlan0     Link encap:Ethernet  HWaddr 00:0A:F5:6D:41:90
          inet addr:192.168.77.83  Bcast:192.168.77.255  Mask:255.255.255.0
          inet6 addr: fe80::20a:f5ff:fe6d:4190/64 Scope:Link
          UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
          RX packets:2775 errors:0 dropped:0 overruns:0 frame:0
          TX packets:636 errors:0 dropped:0 overruns:0 carrier:0
          collisions:0 txqueuelen:1000
          RX bytes:637267 (622.3 KiB)  TX bytes:68110 (66.5 KiB)



```

Note that whitespace characters in the SSID or password need to be enclosed in double quotes.

## Wifi Connection

To connect your robot to wifi, run the script
```sh
./vic_join_wifi.sh <VICTOR_NNNNNNNN>
```
which will connect your robot to the AnkiTest2 network with a dynamic IP.


If your robot has an entry in vic_join_wifi.sh for connecting to AnkiRobots (with a static IP) use the following syntax:

```sh
./vic_join_wifi.sh <Robot Letter>
```
NOTE: The script is a little brittle. It works best when run about 10 seconds after the robot ID appears on victor's face, after boot.
If there is no configuration for your robot you may need to add one; currently our limit is 10 static robot IPs.
