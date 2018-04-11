#!/usr/bin/env python3

import internal_sdk

import argparse
import sys


def main():
    description = 'Automate connection to Victor.'
    parser = argparse.ArgumentParser(description=description)
    parser.add_argument('-a', '--apmode', action='store_true',
                        help='Enable AP mode for update')
    parser.add_argument('-n', '--network',
                        help="Network for Victor to attempt connection")
    parser.add_argument('-p', '--password',
                        help="Password for provided network")
    parser.add_argument('-o', '--ota',
                        help='Set the ota update address')
    parser.add_argument('-b', '--ble',
                        help="Victor's BLE Address")
    parser.add_argument('-d', '--debug', action='store_true',
                        help='Show all debug logs')
    parser.add_argument('-l', '--log', default='/tmp/internal_sdk.log',
                        help='log file to write to')
    parser.add_argument('-s', '--skip', action='store_true',
                        help='skip directly to ota')
    args = parser.parse_args()

    if args.apmode and (args.network or args.password or args.ota):
        sys.exit("Cannot specify network attributes in ap mode")

    mode = internal_sdk.Mode.AP if args.apmode else internal_sdk.Mode.NETWORK
    if args.skip:
        mode = internal_sdk.Mode.SKIP

    with internal_sdk.Connection(ble=args.ble, log_file=args.log,
                                 mode=mode, network_name=args.network,
                                 network_password=args.password,
                                 ota_address=args.ota,
                                 debug=args.debug):
        pass


if __name__ == "__main__":
    main()
