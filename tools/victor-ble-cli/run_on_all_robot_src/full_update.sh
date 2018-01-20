#!/usr/bin/env expect -f

# Source run_func_on_all_robots for access to run_on_all_robots function
source run_on_all_robot_src/run_func_on_all_robots.sh

# Define all things within a namespace to prevent conflicts with anything in run_on_all_robots
namespace eval FullUpdate {

  # Define update function to be called by run_on_all_robots
  proc update name {
    # Spawn a bash instance to start updating the robot
    spawn bash

    # Source bash_profile for victor aliases
    send "source ~/.bash_profile\r"

    # Stop the processes in case they are running
    send "victor_stop\r"
    expect "stopped"
    expect "stopped"
    expect "stopped"

    sleep 2
    send "${::proj_root}/robot/dfu.sh\r"

    set timeout 7
    expect {
      "read 256" {
        set timeout 20
        expect {
          "Success!" { sleep 3 }
          timeout { 
            puts "DFU Failed"
            send \0x3
            sleep 2
            return
          }
        }
      }
      # Expect first dfu attempt to fail and timeout
      timeout {
        set timeout 7
        expect {
          "read 256" {
            set timeout 20
            expect {
              "Success!" { sleep 3 }
              timeout { 
                puts "Second DFU Failed"
                send \0x3
                sleep 2
                return
              }
            }
          }
          timeout {
            puts "Second DFU attempt failed"
            send \0x3
            sleep 2
            return
          }
        }
      }
    }

    sleep 2
    # Deploy build
    send "${::proj_root}/project/victor/scripts/deploy.sh -c Release\r"
    set timeout 300
    expect "deploy /data/data/com.anki.cozmoengine/config/wpa_supplicant_ankitest2.conf"

    # Deploy assests
    sleep 1
    send "${::proj_root}/project/victor/scripts/deploy-assets.sh ${::proj_root}/_build/android/Release/assets\r"
    set timeout 600
    expect {
      "assets installed to" { }
      "assets available at" { }
    }

    # Restart the processes
    sleep 1
    send "victor_restart\r"
    expect "stopped"
    expect "stopped"
    expect "stopped"

    sleep 2
  }
}

run_on_all_robots FullUpdate::update
