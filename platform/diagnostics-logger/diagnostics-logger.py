#!/usr/bin/env python

import subprocess
import os

cmd_ps = [ "ps", "ps | grep vic-\\|anki" ]
cmd_logcat = [ "logcat", "logcat -d | grep -m 1000 vic-switchboard" ]
cmd_wifi = [ "wifi-config", "cat /data/lib/connman/wifi.config | grep -v Passphrase" ]
cmd_ifconfig = [ "ifconfig", "ifconfig wlan0" ]
cmd_iwconfig = [ "iwconfig", "iwconfig wlan0" ]
cmd_dmesg = [ "dmesg", "dmesg" ]
cmd_top = [ "top", "top -n 1" ]
cmd_pinggateway = [ "ping-gateway", "gateway=$(route | grep default | awk '{ print $2 }') && ping -c 4 $gateway" ]
cmd_pinganki = [ "ping-anki", "ping -c 4 anki.com" ]
cmd_connmanservices = [ "connman-services", "connmanctl services" ]
output_path = "/data/diagnostics/"
cmd_tar = [ "tar", "tar -cvjf " + output_path + "logs.tar.bz2 " + output_path + "logs/ " ]

def run_cmd(cmd):
    try:
        return subprocess.check_output(cmd, shell=True)
    except subprocess.CalledProcessError as e:
        return e.output

def run_commands():
    if not os.path.exists(output_path + "logs/"):
        oldmask = os.umask(000)
        os.makedirs(output_path + "logs/", 0755)
        os.umask(oldmask)

    cmds = [ cmd_ps, 
             cmd_logcat,
             cmd_wifi,
             cmd_ifconfig,
             cmd_iwconfig,
             cmd_dmesg,
             cmd_top,
             cmd_pinggateway,
             cmd_pinganki,
             cmd_connmanservices
            ]

    for cmd in cmds:
        output = run_cmd(cmd[1])

        with open(output_path + "logs/" + cmd[0] + ".txt", 'w') as f:
            f.write(output)

def compress_output():
    if os.path.isfile("/data/boot.log"):
        cmd_tar[1] += "/data/boot.log"

    subprocess.check_output(cmd_tar[1].split())

def delete_output():
    if not os.path.exists(output_path + "logs/"):
        print("logs directory doesn't exist")
        return

    if os.path.isfile(output_path + "logs.tar.bz2"):
        os.remove(output_path + "logs.tar.bz2")

    files = [ f for f in os.listdir(output_path + "logs/") if f.endswith(".txt") ]
    for f in files:
        print("deleting log files")
        os.remove(os.path.join(output_path + "logs/", f))

def main():
    delete_output()
    run_commands()
    compress_output()
    exit(0)

#
# entry point
#
if __name__ == "__main__":
    main()
