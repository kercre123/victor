#!/usr/bin/env expect -f

# Source run_on_all_robots for access to run_on_all_robots function
source run_on_all_robot_src/run_func_on_all_robots.sh

# Define all things within a namespace to prevent conflicts with anything in run_on_all_robots
namespace eval Download {

  # Set dir to the current date/time
  set dir [clock format [clock seconds] -format {%Y-%m-%d_%H:%M:%S}]

  # Define download function to be called by run_on_all_robots
  proc download name {
    # Spawn a bash instance to download logs
    spawn bash

    # Source bash_profile for victor aliases
    send "source ~/.bash_profile\r"

    # Stop the processes in case they are running
    send "victor_stop\r"
    set timeout 5
    expect "stopped"
    set timeout 5
    expect "stopped"
    set timeout 5
    expect "stopped"

    # Create Logs/<cur date>/<robot name> directories on desktop
    sleep 1
    # Have to use fully qualified name to refer to namespaced variables since this function is called from outside
    # this namespace
    send "mkdir -p ~/Desktop/Logs/$Download::dir/$name\r"

    # Pull factory logs off robot into newly created directory
    sleep 1
    send "adb pull /data/data/com.anki.cozmoengine/files/output/factory_test_logs/ ~/Desktop/Logs/$Download::dir/$name/\r"
    set timeout 600
    expect {
      "pulled" { }
      "does not exist" { }
    }

    # Restart the processes
    sleep 1
    send "victor_restart\r"
    set timeout 5
    expect "stopped"
    set timeout 5
    expect "stopped"
    set timeout 5
    expect "stopped"

    sleep 1
  }
}

run_on_all_robots Download::download
