#!/usr/bin/env python3

"""
Welcome to Anki's Vector SDK. To use the SDK, you must agree to these terms of service.

// TODO: Legal copy

The removal of this warning, or modification of this script is strictly prohibited.


This script is needed to use the python sdk as of now (when this file was created) because
we have turned on client authorization on the robot. This means that all connections will
require a client token guid to be valid.

Running this script requires that the Robot be on, and connected to the same network as your
laptop. If you have any trouble, please mention it immediately in the #vic-coz-sdk channel.


*IMPORTANT NOTE*

Use _build/mac/Release/bin/tokprovision to provision your robot (not needed if you have used a recent build of Chewie)

****************
"""

import configparser
from getpass import getpass
import json
import os
from pathlib import Path
import requests
import sys

import grpc

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
    if esn is None:
        esn = input('Enter Robot Serial Number: ')
    print("Using Serial:", esn)
    r = requests.get('https://session-certs.token.global.anki-services.com/vic/{}'.format(esn))
    if r.status_code != 200:
        sys.exit(r.content)
    cert = r.content
    return cert, esn

def user_authentication(session_id: bytes, cert: bytes, ip: str, name: str) -> str:
    # Pin the robot certificate for opening the channel
    creds = grpc.ssl_channel_credentials(root_certificates=cert)

    print("Attempting to download guid from {} at {}:443...".format(name, ip))
    channel = grpc.secure_channel("{}:443".format(ip), creds,
                                        options=(("grpc.ssl_target_name_override", name,),))
    interface = api.client.ExternalInterfaceStub(channel)
    request = api.protocol.UserAuthenticationRequest(user_session_id=session_id.encode('utf-8'))
    response = interface.UserAuthentication(request)
    if response.code != api.protocol.UserAuthenticationResponse.AUTHORIZED:
        sys.exit("Failed to authorize request: {}\n\n"
                 "Make sure to follow the logging in to Vector steps first: "
                 "https://ankiinc.atlassian.net/wiki/spaces/VD/pages/449380359/Logging+a+Victor+into+an+Anki+account".format(response))
    print("Successfully retrieved guid")
    return response.client_token_guid

def get_session_token():
    valid = ["prod", "beta", "dev"]
    environ = input("Select an environment [(dev)/beta/prod]? ")
    if environ == "":
        environ = "dev"
    if environ not in valid:
        sys.exit("\nError: That is not a valid environment")

    username = input("Enter Username: ")
    password = getpass("Enter Password: ")
    payload = {'username': username, 'password': password}

    api = Api(environ)
    r = requests.post(api.handler.url, data=payload, headers=api.handler.headers)

    return json.loads(r.content)

def get_name_and_ip():
    robot_name = os.environ['VECTOR_ROBOT_NAME']
    if robot_name is None:
        robot_name = input("Enter Robot Name (ex. Vector-A1B2): ")
    print("Using Robot Name:", robot_name)
    ip = os.environ['ANKI_ROBOT_HOST']
    if ip is None:
        ip = input("Enter Robot IP (ex. 192.168.42.42): ")
    print("Using IP:", ip)
    return robot_name, ip

def main():
    print(__doc__)

    name, ip = get_name_and_ip()
    cert, esn = get_esn()
    token = get_session_token()
    if token.get("session") is None:
        sys.exit("Session error: {}".format(token))
    guid = user_authentication(token["session"]["session_token"], cert, ip, name)

    print("cert :", cert)
    print("esn  :", esn)
    print("guid :", guid)
    print("name :", name)
    print("ip   :", ip)

    # Write cert to a file
    home = Path.home()
    anki_dir = home / ".anki-vector"
    os.makedirs(str(anki_dir), exist_ok=True)
    cert_file = str(anki_dir / "{name}-{esn}.cert".format(name=name, esn=esn))
    print("Writing certificate file to '{}'...".format(cert_file))
    with open(cert_file, 'wb') as f:
        f.write(cert)
    # Store details in a config file
    config_file = str(anki_dir / "sdk_config.ini")
    print("Writing config file to '{}'...".format(config_file))
    config = configparser.ConfigParser()

    config.read(config_file)
    config[esn] = {}
    config[esn]["cert"] = cert_file
    config[esn]["ip"] = ip
    config[esn]["name"] = name
    config[esn]["guid"] = guid.decode("utf-8")
    with open(config_file, 'w') as f:
        config.write(f)
    print("DONE!")

if __name__ == "__main__":
    main()
