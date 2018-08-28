#!/usr/bin/env python3

import argparse
import configparser
import json
import os
from pathlib import Path
import requests
from requests_toolbelt.adapters import host_header_ssl
import sys

try:
    # Non-critical import to add color output
    from termcolor import colored
except:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

try:
    from anki_vector.util import parse_test_args
    from google.protobuf.json_format import MessageToJson, Parse, MessageToDict
    from anki_vector.messaging.protocol import EventRequest, EventResponse
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")


class Connection:
    def __init__(self, serial, port):
        config_file = str(Path.home() / ".anki-vector" / "sdk_config.ini")
        config = configparser.ConfigParser()
        config.read(config_file)
        self.info = {**config[serial]}
        self.port = port
        self.proxies = {'https://{}'.format(self.info["name"]): 'https://{}:{}'.format(self.info["ip"], port)}
        print(self.info)
        self.session = requests.Session()
        self.session.mount("https://", host_header_ssl.HostHeaderSSLAdapter())

    @staticmethod
    def default_callback(response, response_type):
        print("Default response: {}".format(colored(response.content, "cyan")))

    @staticmethod
    def default_stream_callback(response, response_type):
        for r in response.iter_lines():
            data = json.loads(r.decode('utf8'))
            print("Stream response: {}".format(colored(json.dumps(data, indent=4, sort_keys=True), "cyan")))
            print("Converted Protobuf: {}".format(Parse(json.dumps(data["result"]), response_type, ignore_unknown_fields=True).event))


    def send(self, url_suffix, message, response_type, stream=False, callback=None):
        if callback is None:
            callback = Connection.default_callback if not stream else Connection.default_stream_callback
        data = MessageToJson(message, including_default_value_fields=True)
        url = "https://{}:{}/{}".format(self.info["ip"], self.port, url_suffix)
        print(f"{url}: {self.info['guid']} & {self.info['name']}")
        with self.session.post(url, data, stream=stream, verify=self.info["cert"], headers={'Host': self.info["name"],'Authorization': 'Bearer {}'.format(self.info["guid"])}) as r:
            callback(r, response_type)


def main(conn: Connection):
    conn.send("v1/event_stream", EventRequest(connection_id="10"), EventResponse(), stream=True)

if __name__ == "__main__":
    args = parse_test_args()
    conn = Connection(args.serial, args.port)
    main(conn)
