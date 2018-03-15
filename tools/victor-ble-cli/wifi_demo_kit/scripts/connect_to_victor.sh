#!/usr/bin/env expect -f

set robot     [lindex $argv 0]
set SSID      [lindex $argv 1]
set password  [lindex $argv 2]
set general_timeout 15
set sleep_duration 5

proc ble_scan {} {
	set ble_scan_attempts 5
	global general_timeout
	global sleep_duration
	global robot
	
	puts "Bluetooth scanning for $robot"
	while { $ble_scan_attempts > 0 } {
		sleep $sleep_duration
		send "scan\r"
		set timeout $general_timeout		
		expect {
			"Found ${robot}" {
				puts "Found $robot through ble.\n"
				return
			}
			timeout {
				if { $ble_scan_attempts == "1" } {
					puts "\n\nUnable to find $robot after scanning through ble,\
						try restarting $robot"
					exit
				} else {
					puts "reached timeout when scanning for ble connection, retrying"
					set ble_scan_attempts [ expr $ble_scan_attempts-1 ]
				}
			}	
		} 
	}
	puts "\n\nUnable to find $robot after scanning through ble,\
	      try restarting $robot"
	exit 1
}

proc ble_connection {} {
	set ble_connect_attempts 5
	global general_timeout
	global sleep_duration
	global robot
 	
	while { $ble_connect_attempts > 0 } {
		sleep $sleep_duration
		send "connect $robot\r" 
		set timeout $general_timeout
		expect {
			"Fully connected to $robot" {return}
			"You are already connected" {return}
			timeout {
				if { $ble_connect_attempts == "1" } {
					puts "\n\nReached attempts limit for ble connection.\
		 				Try restarting Victor\n"
					exit
				} else {
					puts "reached timeout when connecting to ble, retrying"
					set ble_connect_attempts [ expr $ble_connect_attempts-1 ]
				}					
			}	
		}
	}
	puts "\n\nReached attempts limit for ble connection.\
		 Try restarting Victor\n"
	exit 1
}

proc wifi_test_connection {} {
	global general_timeout
	global sleep_duration

	puts "IP address assigned, checking connection"
	sleep $sleep_duration
	set timeout $general_timeout
	# Currently, need to set a limit on packets sent, because ping for ble-cli
	# tool will run in the backround indefinitely without the ability to exit out
	send "/system/bin/ping -c 2 8.8.8.8\r" 
	expect {
		"bytes" {
			puts "Got a connection will now exit"
			exit 0
		}
		"unreach" {
			puts "don't have a connection,\
				  please reboot Victor and try the script again"
			exit 1
		}
		timeout {
			puts "Reached timeout for testing wifi connection,\
			      will attempt to connect again"
			return
		}
	}
}

proc wifi_connection_and_test {} {
	set wifi_connect_attempts 5
	set wifi_connect_timeout 500
	global general_timeout
	global sleep_duration
	global robot
	global SSID
	global password

	while { $wifi_connect_attempts > 0 } {
		sleep $sleep_duration
		send "wifi-set-config psk false $SSID $password\r"
		set timeout $wifi_connect_timeout
		expect {
			"ip_address=" {
				puts "IP address assigned, checking connection"
				wifi_test_connection ; #If successful, will exit script
				set wifi_connect_attempts [ expr $wifi_connect_attempts-1]
			}
			"fail" {
				puts "hit a failure, retrying"
				set wifi_connect_attempts [ expr $wifi_connect_attempts-1]	
			}		
			timeout {
				if { $wifi_connect_attempts == "1" } {
					puts "Reached maximum amount of tries to connect through wifi.\
	      				Try restarting victor"
					exit
				} else {
					puts "reached timeout when attempting to establish wifi, retrying\r"
					set wifi_connect_attempts [ expr $wifi_connect_attempts-1]
				}					
			}
		}
	}
	puts "Reached maximum amount of tries to connect through wifi.\
	      Try restarting victor"
	exit 1
}	


#Main Program
#Will start victor-ble-cli javascript 
#program in terminal, which is used to interact with victor
#The following functions will be manually sending commands to
#the javascript program 

spawn node ../index.js

ble_scan
ble_connection
wifi_connection_and_test
exit 0



