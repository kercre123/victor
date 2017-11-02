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
$ wifi-set-config SSID PASSWORD
$
Using interface 'wlan0'
OK

$
Heartbeat 0
$

$
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
