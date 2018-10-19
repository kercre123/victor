#!/usr/bin/env python3
# encoding: utf-8

import json
import os
import logging

CURRENT_PATH = os.path.dirname(os.path.realpath(__file__))
DEVICE_INFORMATION_FILE = os.path.join(CURRENT_PATH, "device_information.json")

class Logger :

  def get_logger(name):
    logger = logging.getLogger(name)
    logger.setLevel(logging.INFO)
    console = logging.StreamHandler() #for now, print to just console
    formatter = logging.Formatter('%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    console.setFormatter(formatter)
    logger.addHandler(console)
    return logger
    

class DevicesInfo :

  def __init__(self):
    self.logger = Logger.get_logger(__name__)
  

  def get_device_name(self, serial_id):
    result = ""
    data = {}
    with open(DEVICE_INFORMATION_FILE) as dataFile:
      data = json.loads(dataFile.read())
    try:
      result = "{} - {}".format(data[serial_id]["device id"], data[serial_id]["device name"])
    except:
      self.logger.info("Can't find device, please add device info for serial number: {}".format(serial_id))
      result = serial_id
    return result

  def get_serial_id(self, asset_id):
    data = {}
    with open(DEVICE_INFORMATION_FILE) as dataFile:
      data = json.loads(dataFile.read())
    for item in data:
      if asset_id.lower() in item["device id"]:
         return item
    self.logger.info("Can't find device, please add device info for Asset id: {}".format(asset_id))
    return ""
