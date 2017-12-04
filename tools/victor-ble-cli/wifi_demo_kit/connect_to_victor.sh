#!/usr/bin/env expect -f

set robot     [lindex $argv 0]
set SSID      [lindex $argv 1]
set password  [lindex $argv 2]
set number_of_tries 5

while { $number_of_tries > 0 } {
	spawn node ../index.js

	sleep 5
	send "scan\r"
	set timeout 15
	expect "Found $robot\r" 

	sleep 2
	send "connect $robot\r" 
	set timeout 25
	expect "Fully connected to $robot\r"

	sleep 5
	send "wifi-set-config $SSID $password\r"
	set timeout 500
	expect {
		timeout {puts "reached timeout";  set number_of_tries [ expr $number_of_tries-1]}
		"wpa_state=COMPLETED" {puts "WPA State COMPLETED, will now check for a connection"}
	}

	sleep 2
	send "disconnect\r"
	set timeout 10
	expect "Disconnected from $robot\r" 

	sleep 2
	send "quit\r"
	set timeout 10
	expect "" 

	sleep 5
	spawn bash -c "adb shell ping 8.8.8.8"
	expect {
		"bytes" {puts "Got a connection will now exit";exit}
		"unreach" {puts "don't have a connection, will attempt to connect again"; set number_of_tries [ expr $number_of_tries-1]}
	}
}

puts "\n\n\nReached 5 attempts, try unplugging and trying again"

sleep 2
send "quit\r"

