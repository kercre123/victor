import argparse
import configparser
import json
import os
from pathlib import Path
import requests
from requests_toolbelt.adapters import host_header_ssl
import sys

import pytest
try:
    # Non-critical import to add color output
    from termcolor import colored
except:
    def colored(text, color=None, on_color=None, attrs=None):
        return text

try:
    from anki_vector.util import parse_test_args
    from google.protobuf.json_format import MessageToJson, Parse, MessageToDict
    import anki_vector.messaging.protocol as p
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")


class Connection:
    def __init__(self, serial, port=443):
        config_file = str(Path.home() / ".anki-vector" / "sdk_config.ini")
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

    @staticmethod
    def default_stream_callback(response, response_type):
        for i, r in enumerate(response.iter_lines()):
            print("Shawn Test: {}".format(r))
            data = json.loads(r.decode('utf8'))
            print("Stream response: {}".format(colored(json.dumps(data, indent=4, sort_keys=True), "cyan")))
            assert "result" in data
            print("Converted Protobuf: {}".format(Parse(json.dumps(data["result"]), response_type, ignore_unknown_fields=True)))
            if i >= 10:
                return


    def send(self, url_suffix, message, response_type, stream=False, callback=None):
        if callback is None:
            callback = Connection.default_callback if not stream else Connection.default_stream_callback
        data = MessageToJson(message, including_default_value_fields=True)
        url = "https://{}:{}/{}".format(self.info["ip"], self.port, url_suffix)
        print(f"{url}")
        with self.session.post(url, data, stream=stream, verify=self.info["cert"], headers={'Host': self.info["name"],'Authorization': 'Bearer {}'.format(self.info["guid"])}) as r:
            callback(r, response_type)

@pytest.fixture(scope="module")
def vector_connection():
    return Connection("Local", 8443)

def test_event(vector_connection):
    vector_connection.send("v1/event_stream", p.EventRequest(), p.EventResponse(), stream=True)

# NOTE: Crashes when too long. Being temporarily removed by Ross until it works.
# def test_status_history(vector_connection):
#     vector_connection.send("v1/status_history", p.RobotHistoryRequest(), p.RobotHistoryResponse())

def test_protocol_version(vector_connection):
    vector_connection.send("v1/protocol_version", p.ProtocolVersionRequest(), p.ProtocolVersionResponse())

def test_list_animations(vector_connection):
    vector_connection.send("v1/list_animations", p.ListAnimationsRequest(), p.ListAnimationsResponse())

# def test_display_face_image_rgb(vector_connection):
#     vector_connection.send("v1/display_face_image_rgb", p.DisplayFaceImageRGBRequest(), p.DisplayFaceImageRGBResponse())

@pytest.mark.parametrize("request_params", [
    {"control_request": p.ControlRequest(priority=p.ControlRequest.UNKNOWN)},
    {"control_request": p.ControlRequest(priority=p.ControlRequest.OVERRIDE_ALL)},
    {"control_request": p.ControlRequest(priority=p.ControlRequest.TOP_PRIORITY_AI)},
    {"control_release": p.ControlRelease()}
])
def test_assume_behavior_control(vector_connection, request_params):
    vector_connection.send("v1/assume_behavior_control", p.BehaviorControlRequest(**request_params), p.BehaviorControlResponse(), stream=True)

def test_assume_behavior_control_nil(vector_connection):
    with pytest.raises(AssertionError):
        vector_connection.send("v1/assume_behavior_control", p.BehaviorControlRequest(), p.BehaviorControlResponse(), stream=True)

def test_app_intent(vector_connection):
    vector_connection.send("v1/app_intent", p.AppIntentRequest(), p.AppIntentResponse())

def test_cancel_face_enrollment(vector_connection):
    vector_connection.send("v1/cancel_face_enrollment", p.CancelFaceEnrollmentRequest(), p.CancelFaceEnrollmentResponse())

def test_request_enrolled_names(vector_connection):
    vector_connection.send("v1/request_enrolled_names", p.RequestEnrolledNamesRequest(), p.RequestEnrolledNamesResponse())

def test_update_enrolled_face_by_id(vector_connection):
    vector_connection.send("v1/update_enrolled_face_by_id", p.UpdateEnrolledFaceByIDRequest(), p.UpdateEnrolledFaceByIDResponse())

def test_erase_enrolled_face_by_id(vector_connection):
    vector_connection.send("v1/erase_enrolled_face_by_id", p.EraseEnrolledFaceByIDRequest(), p.EraseEnrolledFaceByIDResponse())

def test_erase_all_enrolled_faces(vector_connection):
    vector_connection.send("v1/erase_all_enrolled_faces", p.EraseAllEnrolledFacesRequest(), p.EraseAllEnrolledFacesResponse())

def test_set_face_to_enroll(vector_connection):
    vector_connection.send("v1/set_face_to_enroll", p.SetFaceToEnrollRequest(), p.SetFaceToEnrollResponse())

def test_enable_vision_mode(vector_connection):
    vector_connection.send("v1/enable_vision_mode", p.EnableVisionModeRequest(), p.EnableVisionModeResponse())

def test_go_to_pose(vector_connection):
    vector_connection.send("v1/go_to_pose", p.GoToPoseRequest(), p.GoToPoseResponse())

def test_dock_with_cube(vector_connection):
    vector_connection.send("v1/dock_with_cube", p.DockWithCubeRequest(), p.DockWithCubeResponse())

# NOTE: hangs forever (needs behavior control maybe)
# def test_drive_off_charger(vector_connection):
#     vector_connection.send("v1/drive_off_charger", p.DriveOffChargerRequest(), p.DriveOffChargerResponse())

# NOTE: hangs forever (needs behavior control maybe)
# def test_drive_on_charger(vector_connection):
#     vector_connection.send("v1/drive_on_charger", p.DriveOnChargerRequest(), p.DriveOnChargerResponse())

def test_pang(vector_connection):
    vector_connection.send("v1/pang", p.Ping(), p.Pong())

def test_get_onboarding_state(vector_connection):
    vector_connection.send("v1/get_onboarding_state", p.OnboardingStateRequest(), p.OnboardingStateResponse())

def test_send_onboarding_input(vector_connection):
    vector_connection.send("v1/send_onboarding_input", p.OnboardingInputRequest(), p.OnboardingInputResponse())

def test_photos_info(vector_connection):
    vector_connection.send("v1/photos_info", p.PhotosInfoRequest(), p.PhotosInfoResponse())

def test_photo(vector_connection):
    vector_connection.send("v1/photo", p.PhotoRequest(), p.PhotoResponse())

def test_thumbnail(vector_connection):
    vector_connection.send("v1/thumbnail", p.ThumbnailRequest(), p.ThumbnailResponse())

def test_delete_photo(vector_connection):
    vector_connection.send("v1/delete_photo", p.DeletePhotoRequest(), p.DeletePhotoResponse())

def test_get_latest_attention_transfer(vector_connection):
    vector_connection.send("v1/get_latest_attention_transfer", p.LatestAttentionTransferRequest(), p.LatestAttentionTransferResponse())

def test_pull_jdocs(vector_connection):
    vector_connection.send("v1/pull_jdocs", p.PullJdocsRequest(), p.PullJdocsResponse())

def test_update_settings(vector_connection):
    vector_connection.send("v1/update_settings", p.UpdateSettingsRequest(), p.UpdateSettingsResponse())

def test_update_account_settings(vector_connection):
    vector_connection.send("v1/update_account_settings", p.UpdateAccountSettingsRequest(), p.UpdateAccountSettingsResponse())

def test_update_user_entitlements(vector_connection):
    vector_connection.send("v1/update_user_entitlements", p.UpdateUserEntitlementsRequest(), p.UpdateUserEntitlementsResponse())

def test_user_authentication(vector_connection):
    vector_connection.send("v1/user_authentication", p.UserAuthenticationRequest(), p.UserAuthenticationResponse())

def test_set_backpack_lights(vector_connection):
    vector_connection.send("v1/set_backpack_lights", p.SetBackpackLightsRequest(), p.SetBackpackLightsResponse())

def test_battery_state(vector_connection):
    vector_connection.send("v1/battery_state", p.BatteryStateRequest(), p.BatteryStateResponse())

def test_version_state(vector_connection):
    vector_connection.send("v1/version_state", p.VersionStateRequest(), p.VersionStateResponse())

def test_network_state(vector_connection):
    vector_connection.send("v1/network_state", p.NetworkStateRequest(), p.NetworkStateResponse())

def test_say_text(vector_connection):
    vector_connection.send("v1/say_text", p.SayTextRequest(), p.SayTextResponse())

# NOTE: Sometimes hangs
def test_connect_cube(vector_connection):
    vector_connection.send("v1/connect_cube", p.ConnectCubeRequest(), p.ConnectCubeResponse())

def test_disconnect_cube(vector_connection):
    vector_connection.send("v1/disconnect_cube", p.DisconnectCubeRequest(), p.DisconnectCubeResponse())

def test_cubes_available(vector_connection):
    vector_connection.send("v1/cubes_available", p.CubesAvailableRequest(), p.CubesAvailableResponse())

def test_flash_cube_lights(vector_connection):
    vector_connection.send("v1/flash_cube_lights", p.FlashCubeLightsRequest(), p.FlashCubeLightsResponse())

def test_forget_preferred_cube(vector_connection):
    vector_connection.send("v1/forget_preferred_cube", p.ForgetPreferredCubeRequest(), p.ForgetPreferredCubeResponse())

def test_set_preferred_cube(vector_connection):
    vector_connection.send("v1/set_preferred_cube", p.SetPreferredCubeRequest(), p.SetPreferredCubeResponse())

# NOTE: Hangs forever (need to turn on camera feed first)
# def test_camera_feed(vector_connection):
#     vector_connection.send("v1/camera_feed", p.CameraFeedRequest(), p.CameraFeedResponse(), stream=True)

def test_check_update_status(vector_connection):
    vector_connection.send("v1/check_update_status", p.CheckUpdateStatusRequest(), p.CheckUpdateStatusResponse())

def test_update_and_restart(vector_connection):
    vector_connection.send("v1/update_and_restart", p.UpdateAndRestartRequest(), p.UpdateAndRestartResponse())

def test_upload_debug_logs(vector_connection):
    vector_connection.send("v1/upload_debug_logs", p.UploadDebugLogsRequest(), p.UploadDebugLogsResponse())

# NOTE: Hangs forever (not implemented in webots nor robot yet)
# def test_check_cloud_connection(vector_connection):
#     vector_connection.send("v1/check_cloud_connection", p.CheckCloudRequest(), p.CheckCloudResponse())
