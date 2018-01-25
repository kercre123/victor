#!/usr/bin/env python3

import sys
import json

json_response=json.load(sys.stdin)

if 'errors' in json_response:
  print("error: " + json.dumps(json_response['errors'][0]))

elif 'html_url' not in json_response:
  print("error: html_url not found in curl response " + json.dumps(json_response))

else:
  print(json_response['html_url'])
