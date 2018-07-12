#!/usr/bin/env python3

from google.protobuf.json_format import MessageToJson

from vector.messaging import settings_pb2

jsonObj = MessageToJson(settings_pb2.RobotSettingsConfig(clock_24_hour=True,
    eye_color=settings_pb2.TEAL, default_location="", dist_is_metric=True,
    locale="en-US", master_volume=settings_pb2.MEDIUM, temp_is_fahrenheit=True,
    time_zone="America/Los_Angeles"), including_default_value_fields=True, preserving_proto_field_name=True)
print(jsonObj)