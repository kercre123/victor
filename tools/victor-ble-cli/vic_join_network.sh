#!/usr/bin/env expect -f

set robot [lindex $argv 0]
set networkSSID [lindex $argv 1]
set networkPSK [lindex $argv 2]
set desiredip [lindex $argv 3]
set defaultGateway [lindex $argv 4]

if { $robot == "" || $networkSSID == "" || $networkPSK == "" || $desiredip == "" || $defaultGateway == ""} {
       puts "Usage: <robot name> <networkSSID> <networkPSK> <static ip> <default gateway>\n"
       exit 1
}

spawn node ./index.js

sleep 2
send "scan\r"
set timeout 15
expect {
    "Found $robot" {}
    timeout {
        send "quit\r"
        puts "Robot $robot not found."
        exit
    }
}

send "connect $robot\r"
set timeout 10
expect {
  "Fully connected to $robot" {}
  timeout {
    send "quit\r"
    puts "Unable to connect to $robot"
    exit
  }
}

sleep 1
send "wpa_cli add_network\r"
expect "Using interface 'wlan0'"
expect -re "(\\d+)" {
    set result $expect_out(1,string)
}

sleep 1
send "wpa_cli set_network $result ssid \"$networkSSID\"\r"
expect "Using interface 'wlan0'"

sleep 1
send "wpa_cli set_network $result psk \"$networkPSK\"\r"
expect "Using interface 'wlan0'"

sleep 1
send "wpa_cli set_network $result scan_ssid 1\r"
expect "Using interface 'wlan0'"

sleep 1
send "wpa_cli disable_network 0\r"
expect "Using interface 'wlan0'"

sleep 1
send "ifconfig wlan0 $desiredip netmask 255.255.252.0\r"

sleep 1
send "ip route add default via $defaultGateway\r"

sleep 1
send "wpa_cli select_network $result\r"
expect "Using interface 'wlan0'"

sleep 1
send "quit\r"


#TODO HANDLE THESE BAD CASES:
#Trying to connect to VICTOR_1dcae602....
#$ wpa_cli add_network

#Not connected to a Victor
