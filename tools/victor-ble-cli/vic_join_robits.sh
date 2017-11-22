#!/usr/bin/env expect -f

set robot [lindex $argv 0]
set desiredip [lindex $argv 1]

if { $robot == "" || $desiredip == "" } {
       puts "Usage: <robot name> <static ip>\n"
       exit 1
}

spawn node ./index.js

sleep 2
send "scan\r"
set timeout 10
expect "Found $robot"

send "connect $robot\r"
expect "Fully connected to $robot"

sleep 1
send "wpa_cli add_network\r"
expect "Using interface 'wlan0'"
expect -re "(\\d+)" {
    set result $expect_out(1,string)
}

sleep 1
send "wpa_cli set_network $result ssid \"AnkiRobits\"\r"
expect "Using interface 'wlan0'"

sleep 1
send "wpa_cli set_network $result psk 827b1d2770b83c5082156c88d8ffd5ffe916e318ec78eb3702cbd4f0a54aed62\r"
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
send "ip route add default via 192.168.40.1\r"

sleep 1
send "wpa_cli select_network $result\r"
expect "Using interface 'wlan0'"

sleep 1
send "quit\r"


#TODO HANDLE THESE BAD CASES:
#Trying to connect to VICTOR_1dcae602....
#$ wpa_cli add_network

#Not connected to a Victor
