
#!/usr/bin/env expect -f

spawn node ./index.js

array set robots {}
set i 0

sleep 2
send "scan\r"
set timeout 10
expect {
  "Found " { 
    expect -re "VICTOR_\[0-9a-f]*" {
      set robot $expect_out(0,string)
      set robots($i) $robot
      incr i
      exp_continue
    }
  }
}

array set robotToIP {}

foreach name [array names robots] {
  set found 0

  if { $robots($name) ne "VICTOR_2ecbdf7d" && $robots($name) ne "VICTOR_ed3dd857"} {
    continue
  }

  sleep 1
  send "scan\r"
  set timeout 10
  expect {
    "Found " { 
      expect -re "VICTOR_\[0-9a-f]*" {
        set robot $expect_out(0,string)
        if { $robot ne $robots($name) } {
          exp_continue
        } else {
          set found 1
        }
      }
    }
  }

  if { $found == 0 } {
    continue
  }

  send "connect $robots($name)\r"
  expect "Fully connected to $robots($name)"

  sleep 2
  send "wifi-set-config AnkiTest2 password\r"
  set timeout 30
  expect "ip_address="
  expect {
    -re "\[0-9]*\.\[0-9]*\.\[0-9]*\.\[0-9]*" {
      set ip $expect_out(0,string)
      set robotToIP($robots($name)) $ip

      sleep 1
      send "disconnect\r"
    }
    timeout {
      continue
    }
  }
  
}

sleep 1
send "quit\r"

foreach name [array names robotToIP] {
  puts "$name $robotToIP($name)"
}

foreach name [array names robotToIP] {
  puts "$name $robotToIP($name)"

  spawn adb connect $robotToIP($name)
  expect {
    "connected to $robotToIP($name)" {
      sleep 1
    }
    "unable to connect" {
      sleep 1
      continue
    }
  }

  spawn adb shell

  sleep 1
  send "./system/bin/display\r"
  expect "reading"

  send "2 2 w $robotToIP($name)\r"
  expect "ok"

  send \0x3

  sleep 1
  send "exit\r"

  # Start updating
  spawn bash
  send "source ~/.bash_profile\r"
  send "victor_stop\r"
  expect "stopped"
  expect "stopped"
  expect "stopped"

  sleep 1
  send "git rev-parse --show-toplevel\r"
  expect "\/Users\/*\/victor"
  set proj_root $expect_out(0,string)

  send "${proj_root}/project/victor/scripts/deploy.sh -c Release\r"
  expect "deploy /data/data/com.anki.cozmoengine/config/wpa_supplicant_ankitest2.conf"

  sleep 1
  send "${proj_root}/project/victor/scripts/deploy-assets.sh ${proj_root}/_build/android/Release/assets\r"
  set timeout 600
  expect {
    "assets installed to" { }
    "assets available at" { }
  }

  sleep 1
  send "victor_restart\r"
  expect "stopped"
  expect "stopped"
  expect "stopped"

  sleep 1
  spawn adb disconnect
  expect "disconnected everything"

  sleep 1 
}
