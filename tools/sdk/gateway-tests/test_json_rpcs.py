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

from util import vector_connection

# NOTE: Crashes when too long. Being temporarily removed by Ross until it works.
# def test_status_history(vector_connection):
#     vector_connection.send("v1/status_history", p.RobotHistoryRequest(), p.RobotHistoryResponse())

def test_protocol_version(vector_connection):
    vector_connection.send("v1/protocol_version", p.ProtocolVersionRequest(), p.ProtocolVersionResponse())

def test_list_animations(vector_connection):
    vector_connection.send("v1/list_animations", p.ListAnimationsRequest(), p.ListAnimationsResponse())

# def test_display_face_image_rgb(vector_connection):
#     vector_connection.send("v1/display_face_image_rgb", p.DisplayFaceImageRGBRequest(), p.DisplayFaceImageRGBResponse())

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

def test_get_onboarding_state(vector_connection):
    vector_connection.send("v1/get_onboarding_state", p.OnboardingStateRequest(), p.OnboardingStateResponse())

# NOTE: Missing parameters
# def test_send_onboarding_input(vector_connection):
#     vector_connection.send("v1/send_onboarding_input", p.OnboardingInputRequest(), p.OnboardingInputResponse())

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

def test_check_update_status(vector_connection):
    vector_connection.send("v1/check_update_status", p.CheckUpdateStatusRequest(), p.CheckUpdateStatusResponse())

def test_update_and_restart(vector_connection):
    vector_connection.send("v1/update_and_restart", p.UpdateAndRestartRequest(), p.UpdateAndRestartResponse())

# NOTE: Not implemented yet
# def test_upload_debug_logs(vector_connection):
#     vector_connection.send("v1/upload_debug_logs", p.UploadDebugLogsRequest(), p.UploadDebugLogsResponse())

def test_check_cloud_connection(vector_connection):
    vector_connection.send("v1/check_cloud_connection", p.CheckCloudRequest(), p.CheckCloudResponse())
