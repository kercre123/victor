#!/usr/bin/env python3
# encoding: utf-8

import sys
import requests


# For grabbing Robot stats from Victor webserver use :
#           RobotConfigStats.get_info_robot(<robot_ip>)

class RobotConfigStats(object) : 

    WEB_SERVER_PORT        = 8888
    GET_ENGINE_PATH        = "getenginestats"
    GET_MAIN_PATH          = "getmainrobotinfo"
    HTTP_PROTOCOL          = "http"
    PREFIX_ROBOT_NAME      = "Vector_"
    ROBOT_ID               = "Robot ID"
    ROBOT_NAME             = "Robot Name"
    ROBOT_SN               = "Robot Serial Number"
    ROBOT_BUILD_CONF       = "Build Configuration"
    ROBOT_OS_BUILD_VER     = "OS Build Version"
    ROBOT_BUILD_SHA        = "Build Sha"
    ROBOT_BATTERY_FILTERED = "Battery (filtered)"
    ROBOT_BATTER_RAW       = "Batter (raw)"
    ROBOT_BATTERY_LEVEL    = "Battery level"
    ROBOT_BATTERY_CHARGING = "Battery charging"

    def get_value_at(items, index):
        return_value = ""
        if len(items) > index:
            return_value = items[index]
        return return_value

    def get_id_robot(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 0)

    def get_sn_robot(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 1)

    def get_build_configrution_robot(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 3)

    def get_os_build_version_robot(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 7)

    def get_build_sha(robot_info):
        return "{}".format(RobotConfigStats.get_value_at(robot_info, 8))

    def get_name_robot(robot_info):
        name = ""
        try:
            name = next(item for item in robot_info \
                        if item.startswith(RobotConfigStats.PREFIX_ROBOT_NAME)
                       )
        except Exception as e:
            print(e)
        return name

    def get_battery_filtered(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 0)

    def get_batter_raw(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 1)

    def get_battery_level(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 2)

    def get_battery_charging(robot_info):
        return RobotConfigStats.get_value_at(robot_info, 3)

    main_robot_handler   =  {
                                ROBOT_ID                : get_id_robot,
                                ROBOT_NAME              : get_name_robot,
                                ROBOT_SN                : get_sn_robot,
                                ROBOT_BUILD_CONF        : get_build_configrution_robot,
                                ROBOT_OS_BUILD_VER      : get_os_build_version_robot,
                                ROBOT_BUILD_SHA         : get_build_sha
                            }

    engine_robot_handler =  {
                                ROBOT_BATTERY_FILTERED  : get_battery_filtered,
                                ROBOT_BATTER_RAW        : get_batter_raw,
                                ROBOT_BATTERY_LEVEL     : get_battery_level,
                                ROBOT_BATTERY_CHARGING  : get_battery_charging
                            }

    @classmethod
    def get_robot_server(self, robot_ip):
        return "{}://{}:{}".format(self.HTTP_PROTOCOL, robot_ip, self.WEB_SERVER_PORT)

    @classmethod
    def get_data_from_url(self, url):
        response = requests.request("GET", url)
        return response.text

    @classmethod
    def get_main_info_robot(self, robot_ip):
        robot_info = {}
        web_server_url = "{}/{}".format(self.get_robot_server(robot_ip), self.GET_MAIN_PATH)
        main_info = self.get_data_from_url(web_server_url)
        if len(main_info) > 0:
            items = main_info.split("\n")
            keys = self.main_robot_handler.keys()
            for key in keys:
                item = { key : (self.main_robot_handler[key](items))}
                robot_info.update(item)
        return robot_info

    @classmethod
    def get_engine_info_robot(self, robot_ip):
        robot_info = {}
        web_server_url = "{}/{}".format(self.get_robot_server(robot_ip), self.GET_ENGINE_PATH)
        engine_info = self.get_data_from_url(web_server_url)
        if len(engine_info) > 0:
            items = engine_info.split("\n")
            keys = self.engine_robot_handler.keys()
            for key in keys:
                item = { key : self.engine_robot_handler[key](items)}
                robot_info.update(item)
        return robot_info

    @classmethod
    def get_info_robot(self, robot_ip):
        robot_info = {}
        main_info = self.get_main_info_robot(robot_ip)
        engine_info = self.get_engine_info_robot(robot_ip)
        robot_info.update(main_info)
        robot_info.update(engine_info)
        return robot_info
