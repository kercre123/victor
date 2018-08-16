#!/usr/bin/env python3

"""
Welcome to Anki's Vector SDK. To use the SDK, you must agree to these terms of service.

// TODO: Legal copy?

The removal of this warning, or modification of this script is strictly prohibited.
"""

from getpass import getpass
import requests
import sys

production = False
beta = False
dev = True

print(__doc__)
valid = ["yes", "y"]
agreed = input("Do you accept the terms of the SDK [y/n]? ") in valid
if not agreed:
    sys.exit("\nError: You must agree to the SDK terms to log in to the robot")
username = input("Username: ")
password = getpass("Password: ")
if production:
    headers = {'Anki-App-Key': 'aung2ieCho3aiph7Een3Ei'}
    url = 'https://accounts.api.anki.com/1/sessions'
if beta: # TODO: Remove Beta and Dev
    headers = {'Anki-App-Key': 'va3guoqueiN7aedashailo'}
    url = 'https://accounts-beta.api.anki.com/1/sessions'
if dev:
    headers = {'Anki-App-Key': 'eiqu8ae6aesae4vi7ooYi7'}
    url = 'https://accounts-dev2.api.anki.com/1/sessions'

payload = {'username': username, 'password': password}
r = requests.post(url, data=payload, headers=headers)
print(r)
print(r.content)
