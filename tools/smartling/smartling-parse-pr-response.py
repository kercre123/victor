#!/usr/bin/env python3

import sys
import json

def extract_sanitized_json_string( source_json ):
  return json.dumps(source_json).replace("\\\"", "\"").replace("\\n", "").replace("\"", "\'")

json_response=json.load(sys.stdin)

if 'errors' in json_response:
  json_string = extract_sanitized_json_string(json_response['errors'][0])
  print("error: " + json_string)

elif 'html_url' not in json_response:
  json_string = extract_sanitized_json_string(json_response)
  print("error: html_url not found in curl response " + json_string)

else:
  print(json_response['html_url'])
