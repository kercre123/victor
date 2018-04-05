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

Scanning started.....
victor-ble-cli$
Found VICTOR_553261a
victor-ble-cli$ connect

Trying to connect to VICTOR_553261a....
victor-ble-cli$
Connected to VICTOR_553261a
victor-ble-cli$
Services discovered for VICTOR_553261a
victor-ble-cli$
Fully connected to VICTOR_553261a
victor-ble-cli$ wifi-set-config SSID PASSWORD
victor-ble-cli$
Using interface 'wlan0'
OK

victor-ble-cli$
Heartbeat 0
victor-ble-cli$

victor-ble-cli$
Using interface 'wlan0'
bssid=80:2a:a8:84:51:62
freq=2462
ssid=stu
id=0
mode=station
pairwise_cipher=CCMP
group_cipher=CCMP
key_mgmt=WPA2-PSK
wpa_state=COMPLETED
ip_address=192.168.77.70
p2p_device_address=00:0a:f5:4c:98:68
address=00:0a:f5:4c:98:68
uuid=d03a0a30-5470-5fbb-80ff-a569b6f53782

```

Note that whitespace characters in the SSID or password are not currently supported.

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
