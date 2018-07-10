#!/usr/bin/env python3
# encoding: utf-8

import json
import os

CURRENT_PATH = os.path.dirname(os.path.realpath(__file__))
DEVICE_INFORMATION_FILE = os.path.join(CURRENT_PATH, "device_information.json")

class DevicesInfo :

  def get_device_name(serial_id):
    result = ""
    data = {}
    with open(DEVICE_INFORMATION_FILE) as dataFile:
      data = json.loads(dataFile.read())
    try:
      result = "{} - {}".format(data[serial_id]["device id"], data[serial_id]["device name"])
    except:
      print("Can't find device, please add device info for serial number: {}".format(serial_id))
      result = serial_id
    return result