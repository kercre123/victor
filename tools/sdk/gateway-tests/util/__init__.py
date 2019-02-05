import base64
import configparser
import json
import os
from pathlib import Path
import sys

try:
    from PIL import Image
    import pytest
    import requests
    from requests_toolbelt.adapters import host_header_ssl
except ImportError:
    sys.exit("\n\nThis script requires you to install 'pytest', 'requests' and 'requests_toolbelt'.\n"
             "To do so, please run '{pip_install}'\n"
             "Then try again".format(
                 pip_install="pip3 install Pillow pytest requests requests_toolbelt",
             ))

try:
    # Non-critical import to add color output
    from termcolor import colored
except ImportError:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

try:
    from google.protobuf.json_format import MessageToJson, Parse
except ImportError:
    sys.exit("Error: This script requires you to run setup.sh")

class Connection:
    def __init__(self, serial):
        config_file = str(Path.home() / ".anki_vector" / "sdk_config.ini")
        config = configparser.ConfigParser()
        config.read(config_file)
        self.info = {**config[serial]}
        if "port" in self.info:
            self.port = self.info["port"]
        else:
            self.port = "443"
        self.proxies = {'https://{}'.format(self.info["name"]): 'https://{}:{}'.format(self.info["ip"], self.port)}
        print(self.info)
        self.session = requests.Session()
        self.session.mount("https://", host_header_ssl.HostHeaderSSLAdapter())

    @staticmethod
    def default_callback(response, response_type):
        print("Default response: {}".format(colored(response.content, "cyan")))
        assert response.status_code == 200, "Received failure status_code: {} => {}".format(response.status_code, response.content)
        Parse(response.content, response_type, ignore_unknown_fields=True)
        print("Converted Protobuf: {}".format(colored(response_type, "cyan")))

    @staticmethod
    def default_stream_callback(response, response_type, iterations=10):
        i = 0
        for i, r in enumerate(response.iter_lines()):
            data = json.loads(r.decode('utf8'))
            print("Stream response: {}".format(colored(json.dumps(data, indent=4, sort_keys=True), "cyan")))
            assert "result" in data
            Parse(json.dumps(data["result"]), response_type, ignore_unknown_fields=True)
            print("Converted Protobuf: {}".format(colored(response_type, "cyan")))
            if i == iterations:
                break
        print("{} of {} iterations".format(i, iterations))
        assert i == iterations, "Stream closed before expected number of iterations"

    def send_raw(self, url_suffix, data, response_type, stream=None, callback=None):
        print()
        if callback is None:
            callback = Connection.default_callback if not stream else Connection.default_stream_callback
        url = "https://{}:{}/{}".format(self.info["ip"], self.port, url_suffix)
        print(f"Sending to {colored(url, 'cyan')} <- {colored(data, 'cyan')}")
        print("---")
        with self.session.post(url, data, stream=stream is not None, verify=self.info["cert"], headers={'Host': self.info["name"],'Authorization': 'Bearer {}'.format(self.info["guid"])}) as r:
            callback(r, response_type, **{"iterations": stream} if stream is not None else {})

    def send(self, url_suffix, message, response_type, stream=None, callback=None):
        data = MessageToJson(message, including_default_value_fields=True, preserving_proto_field_name=True)
        return self.send_raw(url_suffix, data, response_type, stream, callback)

def is_local():
    return os.environ.get('ANKI_ROBOT_SERIAL', '') == "Local"

def live_robot_only(fn):
    return pytest.mark.skipif(is_local(), reason="vic-cloud is not available on webots")(fn)

@pytest.fixture(scope="module")
def vector_connection():
    serial = os.environ.get('ANKI_ROBOT_SERIAL', None)
    if serial is None:
        sys.exit("Please set 'ANKI_ROBOT_SERIAL' environment variable with 'export ANKI_ROBOT_SERIAL=<your robot's serial number>'. To run with webots set your serial number to 'Local'")
    return Connection(serial)

def data_from_file(name):
    with open(name, 'rb') as f:
        return f.read()