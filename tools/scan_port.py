#!/usr/bin/env python2
import socket
import subprocess
import sys
from datetime import datetime
import argparse

MIN_PORT = 1
MAX_PORT = 65535

def clear_screen():
    subprocess.call('clear', shell=True)

def scan_port(ip, begin_port, end_port):
    remote_ip  = socket.gethostbyname(ip)
    
    print "Please wait, scanning remote host", remote_ip
    print "-" * 60

    try:
        for port in range(begin_port,end_port):  
            sock = socket.socket()
            result = sock.connect_ex((remote_ip, port))
            if result == 0:
                print "Port {}:      Open".format(port)
            sock.close()

    except KeyboardInterrupt:
        print "You pressed Ctrl+C"
        sys.exit()

    except socket.gaierror:
        print 'Hostname could not be resolved. Exiting'
        sys.exit()

    except socket.error:
        print "Couldn't connect to server"
        sys.exit()

    print "-" * 60
    
def parse_arguments(args):

  parser = argparse.ArgumentParser()
  
  parser.add_argument('-i', '--ip',
                      action='store',
                      required=True,
                      help="Vector's IP for scanning port")
  options = parser.parse_args(args)
  return (parser, options)

def main():
    args = sys.argv[1:]
    parser, options = parse_arguments(args)
    ip = options.ip

    clear_screen()
    start_time = datetime.now()
    scan_port(ip, MIN_PORT, MAX_PORT)
    end_time = datetime.now()

    total =  end_time - start_time
    print 'Scanning Completed in: ', total

if __name__ == "__main__":
    main()
