import configparser
import json
import os
from pathlib import Path
import sys

try:
    import pytest
    import requests
    from requests_toolbelt.adapters import host_header_ssl
except ImportError:
    sys.exit("\n\nThis script requires you to install 'pytest', 'requests' and 'requests_toolbelt'.\n"
             "To do so, please run '{pip_install}'\n"
             "Then try again".format(
                 pip_install="pip3 install pytest requests requests_toolbelt",
             ))

try:
    # Non-critical import to add color output
    from termcolor import colored
except ImportError:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

try:
    from google.protobuf.json_format import MessageToJson, Parse
    import anki_vector.messaging.protocol as p
except ImportError:
    base_dir = Path(os.path.dirname(os.path.realpath(__file__))) / ".." / ".."
    base_dir = base_dir.resolve()
    sys.exit("\n\nThis script requires you to install the Vector SDK.\n"
                "To do so, please navigate to '{tools_path}' and run '{make}'\n"
                "Next navigate to '{sdk_path}', run '{pip_install}', and run '{configure}'\n"
                "Then try again".format(
                    tools_path=str(base_dir / "grpc_tools"),
                    make="make",
                    sdk_path=str(base_dir / "vector-sdk"),
                    pip_install="pip install -e .",
                    configure="python3 configure.py",
                ))

class Connection:
    def __init__(self, serial, port=443):
        config_file = str(Path.home() / ".anki_vector" / "sdk_config.ini")
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
        assert response.status_code == 200, "Received failure status_code: {}".format(response.status_code)
        Parse(response.content, response_type, ignore_unknown_fields=True)
        print("Converted Protobuf: {}".format(colored(response_type, "cyan")))

    @staticmethod
    def default_stream_callback(response, response_type, iterations=10):
        i = 0
        for i, r in enumerate(response.iter_lines()):
            print("Shawn Test: {}".format(r))
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
        data = MessageToJson(message, including_default_value_fields=True)
        return self.send_raw(url_suffix, data, response_type, stream, callback)

@pytest.fixture(scope="module")
def vector_connection():
    if os.environ.get('FORCE_LIVE_ROBOT', None) is not None:
        serial = os.environ.get('ANKI_ROBOT_SERIAL', None)
        if serial is None:
            sys.exit("Please set 'ANKI_ROBOT_SERIAL' or unset 'FORCE_LIVE_ROBOT'")
        return Connection(serial)
    return Connection("Local", port=8443)