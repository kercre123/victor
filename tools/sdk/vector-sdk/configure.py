#!/usr/bin/env python3

#TODO Update file throughout to be good for the public.

"""
***Anki Vector Python SDK Setup***

Vector requires all requests be authorized by an authenticated Anki user.

This script will enable this device to authenticate with your Vector
robot for use with a Vector Python SDK program.

Vector must be powered on and connected on the same network as your
computer. By running this script, you will be asked to provide your
Anki account credentials, and the script will download an authentication
token and cert that will grant you access to the robot and his
capabilities (such as camera and audio) as well as data stored on the
robot (such as faces and photos).

See the README for more information.


"""

import configparser
from getpass import getpass
import json
import os
from pathlib import Path
import requests
import sys

from google.protobuf.json_format import MessageToJson
import grpc
try:
    from termcolor import colored
except:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

import anki_vector.messaging as api

class ApiHandler:
    def __init__(self, headers: dict, url: str):
        self._headers = headers
        self._url = url

    @property
    def headers(self):
        return self._headers

    @property
    def url(self):
        return self._url

class Api:
    def __init__(self, environment):
        if environment == "prod":
            self._handler = ApiHandler(
                headers={'Anki-App-Key': 'aung2ieCho3aiph7Een3Ei'},
                url='https://accounts.api.anki.com/1/sessions'
            )
        # TODO: remove beta and dev from released version
        elif environment == "beta":
            self._handler = ApiHandler(
                headers={'Anki-App-Key': 'va3guoqueiN7aedashailo'},
                url='https://accounts-beta.api.anki.com/1/sessions'
            )
        else:
            self._handler = ApiHandler(
                headers={'Anki-App-Key': 'eiqu8ae6aesae4vi7ooYi7'},
                url='https://accounts-dev2.api.anki.com/1/sessions'
            )

    @property
    def handler(self):
        return self._handler

def get_esn():
    esn = os.environ.get('ANKI_ROBOT_SERIAL')
    if esn is None or len(esn) == 0:
        print("\n\nPlease find your robot serial number (ex. 00e20100) located on the underside of Vector, or accessible from Vector's debug screen.")
        esn = input('Enter robot serial number: ')
    else:
        print("Found robot serial number in environment variable '{}'".format(colored("ANKI_ROBOT_SERIAL", "green")))
    esn = esn.lower()
    print("Using robot serial number: {}".format(colored(esn, "cyan")))
    print("\nDownloading Vector certificate from Anki...", end="")
    sys.stdout.flush()
    r = requests.get('https://session-certs.token.global.anki-services.com/vic/{}'.format(esn))
    if r.status_code != 200:
        print(colored(" ERROR", "red"))
        sys.exit(r.content)
    print(colored(" DONE\n", "green"))
    cert = r.content
    return cert, esn

def user_authentication(session_id: bytes, cert: bytes, ip: str, name: str) -> str:
    # Pin the robot certificate for opening the channel
    creds = grpc.ssl_channel_credentials(root_certificates=cert)

    print("Attempting to download guid from {} at {}:443...".format(colored(name, "cyan"), colored(ip, "cyan")), end="")
    sys.stdout.flush()
    channel = grpc.secure_channel("{}:443".format(ip), creds,
                                        options=(("grpc.ssl_target_name_override", name,),))
    interface = api.client.ExternalInterfaceStub(channel)
    request = api.protocol.UserAuthenticationRequest(user_session_id=session_id.encode('utf-8'))
    response = interface.UserAuthentication(request)
    if response.code != api.protocol.UserAuthenticationResponse.AUTHORIZED:
        print(colored(" ERROR", "red"))
        sys.exit("Failed to authorize request: {}\n\n"
                 "Please be sure to connect via the Vector companion app and log into a Vector into an account first.")
    print(colored(" DONE\n", "green"))
    return response.client_token_guid

# TODO Remove everything that is not prod
def get_session_token():
    valid = ["prod", "beta", "dev"]
    environ = input("Select an environment [(dev)/beta/prod]? ")
    if environ == "":
        environ = "dev"
    if environ not in valid:
        sys.exit("{}: That is not a valid environment".format(colored("ERROR", "red")))

    print("Enter your email and password. Make sure to use the same account that was used to set up your Vector.")
    username = input("Enter Email: ")
    password = getpass("Enter Password: ")
    payload = {'username': username, 'password': password}

    print("\nAuthenticating with Anki...", end="")
    sys.stdout.flush()
    api = Api(environ)
    r = requests.post(api.handler.url, data=payload, headers=api.handler.headers)
    if r.status_code != 200:
        print(colored(" ERROR", "red"))
        sys.exit(r.content)
    print(colored(" DONE\n", "green"))
    return json.loads(r.content)

def get_name_and_ip():
    robot_name = os.environ.get('VECTOR_ROBOT_NAME')
    if robot_name is None or len(robot_name) == 0:
        print("\n\nFind your robot name (ex. Vector-A1B2) by placing Vector on the charger and double clicking Vector's backpack button.")
        robot_name = input("Enter robot name: ")
    else:
        print("Found robot name in environment variable '{}'".format(colored("VECTOR_ROBOT_NAME", "green")))
    print("Using robot name: {}".format(colored(robot_name, "cyan")))
    ip = os.environ.get('ANKI_ROBOT_HOST')
    if ip is None or len(ip) == 0:
        print("\n\nFind your robot ip address (ex. 192.168.42.42) by placing Vector on the charger, double clicking Vector's backpack button, then raising and lowering his arms.\n"
              "If you see {} on his face, reconnect Vector to your WiFi using the Vector Companion App.".format(colored("XX.XX.XX.XX", "red")))
        ip = input("Enter robot ip: ")
    else:
        print("Found robot ip address in environment variable '{}'".format(colored("ANKI_ROBOT_HOST", "green")))
    print("Using IP: {}".format(colored(ip, "cyan")))
    return robot_name, ip

def main():
    print(__doc__)

    valid = ["y", "Y", "yes", "YES"]
    environ = input("Do you wish to proceed? (y/n) ")
    if environ not in valid:
        sys.exit("Stopping...")

    name, ip = get_name_and_ip()
    cert, esn = get_esn()
    token = get_session_token()
    if token.get("session") is None:
        sys.exit("Session error: {}".format(token))

    # Write cert to a file located in user's home direction: sdk_config.ini
    home = Path.home()
    anki_dir = home / ".anki_vector"
    os.makedirs(str(anki_dir), exist_ok=True)
    cert_file = str(anki_dir / "{name}-{esn}.cert".format(name=name, esn=esn))
    print("Writing certificate file to '{}'...".format(colored(cert_file, "cyan")))
    with os.fdopen(os.open(cert_file, os.O_WRONLY | os.O_CREAT, 0o600), 'wb') as f:
        f.write(cert)

    guid = user_authentication(token["session"]["session_token"], cert, ip, name)

    # Store details in a config file
    config_file = str(anki_dir / "sdk_config.ini")
    print("Writing config file to '{}'...".format(colored(config_file, "cyan")))

    config = configparser.ConfigParser(strict=False)

    try:
        config.read(config_file)
    except configparser.ParsingError:
        if os.path.exists(config_file):
            os.rename(config_file, config_file + "-error")
    config[esn] = {}
    config[esn]["cert"] = cert_file
    config[esn]["ip"] = ip
    config[esn]["name"] = name
    config[esn]["guid"] = guid.decode("utf-8")
    temp_file = config_file + "-temp"
    if os.path.exists(config_file):
        os.rename(config_file, temp_file)
    try:
        with os.fdopen(os.open(config_file, os.O_WRONLY | os.O_CREAT, 0o600), 'w') as f:
            config.write(f)
    except Exception as e:
        if os.path.exists(temp_file):
            os.rename(temp_file, config_file)
        raise e
    else:
        if os.path.exists(temp_file):
            os.remove(temp_file)
    print(colored("SUCCESS!", "green"))

if __name__ == "__main__":
    main()
