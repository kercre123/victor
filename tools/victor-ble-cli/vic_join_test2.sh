#!/usr/bin/env expect -f

set robot [lindex $argv 0]

if { $robot == "" } {
       puts "Usage: <robot name>\n"
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
set timeout 180
send "wifi-set-config psk false AnkiTest2 password\r"
expect "wlan0"

sleep 1
send "quit\r"
