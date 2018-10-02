# This test file runs via pytest, and will test the json endpoints for all of the unary rpcs.
#
# This file reproduces a connection that is similar to the way the app will talk to Vector.
#
import json
import os
import sys

import pytest

try:
    import anki_vector.messaging.protocol as p
except ImportError:
    sys.exit("Error: This script requires you to install the Vector SDK")

from util import vector_connection, json_from_file

# NOTE: Crashes when too long. Being temporarily removed by Ross until it works.
# def test_status_history(vector_connection):
#     vector_connection.send("v1/status_history", p.RobotHistoryRequest(), p.RobotHistoryResponse())

def test_protocol_version(vector_connection):
    vector_connection.send("v1/protocol_version", p.ProtocolVersionRequest(), p.ProtocolVersionResponse())

def test_list_animations(vector_connection):
    vector_connection.send("v1/list_animations", p.ListAnimationsRequest(), p.ListAnimationsResponse())

# TODO: providing default data crashes this rpc, and the oled_color.json file doesn't render (nor did it in the grpc_tools Makefile)
# @pytest.mark.parametrize("data", [
#     json_from_file(os.path.join('assets', 'oled_color.json'))
# ])
# def test_display_face_image_rgb(vector_connection, data):
#     vector_connection.send_raw("v1/display_face_image_rgb", data, p.DisplayFaceImageRGBResponse())

@pytest.mark.parametrize("data", [
    '{"intent":"intent_meet_victor","param":"Bobert"}'
])
def test_app_intent(vector_connection, data):
    vector_connection.send_raw("v1/app_intent", data, p.AppIntentResponse())

def test_cancel_face_enrollment(vector_connection):
    vector_connection.send("v1/cancel_face_enrollment", p.CancelFaceEnrollmentRequest(), p.CancelFaceEnrollmentResponse())

def test_request_enrolled_names(vector_connection):
    vector_connection.send("v1/request_enrolled_names", p.RequestEnrolledNamesRequest(), p.RequestEnrolledNamesResponse())

@pytest.mark.parametrize("data", [
    '{"face_id":1,"old_name":"Bobert","new_name":"Boberta"}'
])
def test_update_enrolled_face_by_id(vector_connection, data):
    vector_connection.send_raw("v1/update_enrolled_face_by_id", data, p.UpdateEnrolledFaceByIDResponse())

@pytest.mark.parametrize("data", [
    '{"face_id":1}'
])
def test_erase_enrolled_face_by_id(vector_connection, data):
    vector_connection.send_raw("v1/erase_enrolled_face_by_id", data, p.EraseEnrolledFaceByIDResponse())

def test_erase_all_enrolled_faces(vector_connection):
    vector_connection.send("v1/erase_all_enrolled_faces", p.EraseAllEnrolledFacesRequest(), p.EraseAllEnrolledFacesResponse())

@pytest.mark.parametrize("data", [
    '{"name":"Boberta","observed_id":1,"save_id":0,"save_to_robot":true,"say_name":true,"use_music":true}'
])
def test_set_face_to_enroll(vector_connection, data):
    vector_connection.send_raw("v1/set_face_to_enroll", data, p.SetFaceToEnrollResponse())

def test_enable_vision_mode(vector_connection):
    vector_connection.send("v1/enable_vision_mode", p.EnableVisionModeRequest(), p.EnableVisionModeResponse())

# TODO: add behavior control or else this does nothing
# @pytest.mark.parametrize("data", [
#     '{"id_tag": 2000001, "x_mm":100.0, "y_mm":100.0, "rad":0.0, "motion_prof":{"speed_mmps":110.0, "accel_mmps2":200.0, "decel_mmps2": 500.0, "point_turn_speed_rad_per_sec": 2.0, "point_turn_accel_rad_per_sec2": 10.0, "point_turn_decel_rad_per_sec2": 10.0, "dock_speed_mmps": 60.0, "dock_accel_mmps2": 200.0, "dock_decel_mmps2": 500.0, "reverse_speed_mmps": 80.0, "is_custom": false}}'
# ])
# def test_go_to_pose(vector_connection, data):
#     vector_connection.send_raw("v1/go_to_pose", data, p.GoToPoseResponse())

# TODO: add behavior control or else this does nothing
# @pytest.mark.parametrize("data", [
#     '{"id_tag": 2000001}'
# ])
# def test_dock_with_cube(vector_connection, data):
#     vector_connection.send_raw("v1/dock_with_cube", data, p.DockWithCubeResponse())

# NOTE: hangs forever (needs behavior control maybe)
# def test_drive_off_charger(vector_connection):
#     vector_connection.send("v1/drive_off_charger", p.DriveOffChargerRequest(), p.DriveOffChargerResponse())

# NOTE: hangs forever (needs behavior control maybe)
# def test_drive_on_charger(vector_connection):
#     vector_connection.send("v1/drive_on_charger", p.DriveOnChargerRequest(), p.DriveOnChargerResponse())

def test_get_onboarding_state(vector_connection):
    vector_connection.send("v1/get_onboarding_state", p.OnboardingStateRequest(), p.OnboardingStateResponse())

# NOTE: Missing parameters
# def test_send_onboarding_input(vector_connection):
#     vector_connection.send("v1/send_onboarding_input", p.OnboardingInputRequest(), p.OnboardingInputResponse())

def test_photos_info(vector_connection):
    vector_connection.send("v1/photos_info", p.PhotosInfoRequest(), p.PhotosInfoResponse())

@pytest.mark.parametrize("data", [
    '{"photo_id":0}',
    '{"photo_id":15}',
])
def test_photo(vector_connection, data):
    vector_connection.send_raw("v1/photo", data, p.PhotoResponse())

@pytest.mark.parametrize("data", [
    '{"thumbnail_id":0}',
    '{"thumbnail_id":15}',
])
def test_thumbnail(vector_connection, data):
    vector_connection.send_raw("v1/thumbnail", data, p.ThumbnailResponse())

@pytest.mark.parametrize("data", [
    '{"photo_id":0}',
    '{"photo_id":15}',
])
def test_delete_photo(vector_connection, data):
    vector_connection.send_raw("v1/delete_photo", data, p.DeletePhotoResponse())

def test_get_latest_attention_transfer(vector_connection):
    vector_connection.send("v1/get_latest_attention_transfer", p.LatestAttentionTransferRequest(), p.LatestAttentionTransferResponse())

@pytest.mark.parametrize("data", [
    '{"jdoc_types":["ROBOT_SETTINGS", "ROBOT_LIFETIME_STATS"]}',
])
def test_pull_jdocs(vector_connection, data):
    vector_connection.send_raw("v1/pull_jdocs", data, p.PullJdocsResponse())

@pytest.mark.parametrize("data", [
    '{"settings": {"default_location":"San Francisco, California"}}',
    '{"settings": {"eye_color": 0}}',
    '{"settings": {"eye_color": 1}}',
    '{"settings": {"eye_color": 2}}',
    '{"settings": {"eye_color": 3}}',
    '{"settings": {"eye_color": 4}}',
    '{"settings": {"eye_color": 5}}',
    '{"settings": {"eye_color": 6}}',
    '{"settings": {"eye_color": 7}}',
    '{"settings": {"eye_color": -1}}',
    '{"settings":{"clock_24_hour":true,"eye_color":5}}',
    '{"settings":{"clock_24_hour":true}}',
])
def test_update_settings_raw(vector_connection, data):
    def callback(response, response_type):
        print("Default response: {}".format(response.content))
        data = json.loads(response.content)
        assert "code" in data
        assert data["code"] == 0
    vector_connection.send_raw("v1/update_settings", data, p.UpdateSettingsResponse(), callback=callback)

def test_update_settings(vector_connection):
    vector_connection.send("v1/update_settings", p.UpdateSettingsRequest(), p.UpdateSettingsResponse())

@pytest.mark.parametrize("data", [
    '{"account_settings": {"app_locale":"en-IN","data_collection":false}}',
    '{"account_settings": {"app_locale":"en-US","data_collection":true}}',
])
def test_update_account_settings(vector_connection, data):
    vector_connection.send_raw("v1/update_account_settings", data, p.UpdateAccountSettingsResponse())

@pytest.mark.parametrize("data", [
    '{"user_entitlements": {"kickstarter_eyes":false}}',
    '{"user_entitlements": {"kickstarter_eyes":true}}',
])
def test_update_user_entitlements(vector_connection, data):
    vector_connection.send_raw("v1/update_user_entitlements", data, p.UpdateUserEntitlementsResponse())

def test_user_authentication(vector_connection):
    vector_connection.send("v1/user_authentication", p.UserAuthenticationRequest(), p.UserAuthenticationResponse())

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

@pytest.mark.parametrize("data", [
    '{"factory_id":"11:11:11:11:11:11"}'
])
def test_set_preferred_cube(vector_connection, data):
    vector_connection.send_raw("v1/set_preferred_cube", data, p.SetPreferredCubeResponse())

def test_check_update_status(vector_connection):
    vector_connection.send("v1/check_update_status", p.CheckUpdateStatusRequest(), p.CheckUpdateStatusResponse())

def test_update_and_restart(vector_connection):
    vector_connection.send("v1/update_and_restart", p.UpdateAndRestartRequest(), p.UpdateAndRestartResponse())

# NOTE: Not implemented yet
# def test_upload_debug_logs(vector_connection):
#     vector_connection.send("v1/upload_debug_logs", p.UploadDebugLogsRequest(), p.UploadDebugLogsResponse())

def test_check_cloud_connection(vector_connection):
    vector_connection.send("v1/check_cloud_connection", p.CheckCloudRequest(), p.CheckCloudResponse())
