#!/usr/bin/env expect -f

set proj_root [lindex $argv 0]

# Runs "func" for each robot in range
# The name of the robot is passed into func, it will look like `VICTOR_<serial number>`
# func is called after `adb connect`ing to the robot, with no active sessions (func is free to spawn things)
proc run_on_all_robots func {
  # List of robots we have already/previously called func on
  # is written to file at end of run
  set completed {}

  # Read in the robots we have already called func on
  if {[file exists "run_on_all_robot_src/$func.txt"]} {
    set f [open "run_on_all_robot_src/$func.txt" "r"]
    set completed [split [read $f] "\n"]
    close $f
  }

  # Print the robots we will be ignoring/skipping
  foreach i $completed {
    puts "Will ignore $i"
  }

  # Start victor node ble cli tool
  spawn node "./index.js"

  # List of found robots. Really a map of int -> VICTOR_<serial number>
  array set robots {}
  set i 0

  # Start scanning for robots
  sleep 2
  send "scan\r"

  # Timeout waiting to find robots 10 seconds after the last found robot
  set timeout 10
  expect {
    "Found " { 
      # When we find a robot, extract name from output
      expect -re "VICTOR_\[0-9a-f]*" {
        # Add robot name to list of found robots
        set robot $expect_out(0,string)
        set robots($i) $robot
        incr i

        # Run outer expect again to find more robots
        # Resets expect timeout
        exp_continue
      }
    }
  }

  # Maps VICTOR_<serial number> -> IP Address
  array set robotToIP {}

  # For each of the found robots, scan for them again and if found connect
  foreach name [array names robots] {
    set found 0

    if { [lsearch $completed $robots($name)] >= 0 } {
      puts "Ignoring $robots($name)\n"
      continue
    }

    # Rescan for the current robot
    sleep 1
    send "scan\r"
    set timeout 10
    expect {
      "Found " { 
        expect -re "VICTOR_\[0-9a-f]*" {
          set robot $expect_out(0,string)

          # If the found robot is not the one we are looking for
          if { $robot ne $robots($name) } {
            # Keep waiting to find it
            exp_continue
          } else {
            # Otherwise leave the "Found" expect and mark as having found the robot
            set found 1
          }
        }
      }
    }

    # Must have timed out scanning for this robot so move onto the next one
    if { $found == 0 } {
      continue
    }

    # We found the robot we were looking for so connect
    send "connect $robots($name)\r"
    expect "Fully connected to $robots($name)"

    # Check if the robot is already on the network and has an ip
    sleep 2
    send "ifconfig\r"
    set timeout 3
    expect { 
      "inet addr:" {
        set timeout 1
        expect {
          -re "192.\[0-9]*\.\[0-9]*\.\[0-9]*" {
            set ip $expect_out(0,string)
            set robotToIP($robots($name)) $ip

            # Robot already has an ip so disconnect
            sleep 1
            send "disconnect\r"
            continue
          }
          timeout { }
        }
      }
      timeout { }
    }

    # Configure to connect to the AnkiTest2 network
    sleep 2
    send "wifi-set-config psk false AndroidAP ftej1581\r"
    set timeout 30
    expect "ip_address="
    expect {
      # Find the ip address of this robot and add it to the robotToIP map
      -re "\[0-9]*\.\[0-9]*\.\[0-9]*\.\[0-9]*" {
        set ip $expect_out(0,string)
        set robotToIP($robots($name)) $ip

        # The robot is now connected to the network and we know its IP so disconnect
        sleep 1
        send "disconnect\r"
      }
      # Configuring wifi took too long so move on to the next robot
      timeout {
        continue
      }
    }
  }

  # We have finished trying to connect to and configuring wifi on the robots
  # so quit the ble tool
  sleep 1
  send "quit\r"

  # Print all the robot names and their IP addresses
  foreach name [array names robotToIP] {
    puts "$name $robotToIP($name)"
  }

  # Disconnect from anything first
  sleep 1
  spawn adb disconnect
  expect "disconnected everything"

  # For each of the robots that have an ip address
  foreach name [array names robotToIP] {
    puts "$name $robotToIP($name)"

    # Connect over adb
    spawn adb connect $robotToIP($name)
    expect {
      "connected to $robotToIP($name)" {
        sleep 3
      }
      "unable to connect" {
        # Couldn't connect so move on to the next one
        sleep 1
        continue
      }
    }

    # Check if device is offline
    spawn adb devices
    expect {
      "\tdevice" { }
      "offline" {
        sleep 1
        spawn adb disconnect
        expect "disconnected everything"
        continue
      }
    }

    # Shell into the robot
    set timeout 2
    spawn adb shell
    expect {
      "no devices" {
        sleep 1
        spawn adb disconnect
        expect "disconnected everything"
        continue
      }
      timeout { }
    }

    # Draw the ip address on the face using the "display" program
    sleep 1
    send "./system/bin/display\r"
    expect "reading"

    send "2 2 w $robotToIP($name)\r"
    expect "ok"

    # Send "ctrl+c" to leave the "display" program
    send \0x3

    # Exit adb shell
    sleep 1
    send "exit\r"

    # Call func and pass in the robot name
    $func $name

    # Disconnect from robot
    sleep 1
    spawn adb disconnect
    expect "disconnected everything"

    # Add this robot to the list of robots we have already called func on
    lappend completed $name

    sleep 1 
  }

  # Write the list of completed robots to file
  set var [open "run_on_all_robot_src/$func.txt" "w"]
  foreach r $completed {
    puts $var "$r"
  }
  close $var

  puts "Done"
}
