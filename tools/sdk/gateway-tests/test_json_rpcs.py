# This test file runs via pytest, and will test the json endpoints for all of the unary rpcs.
#
# This file reproduces a connection that is similar to the way the app will talk to Vector.
#
import json
import os
import sys

import pytest

try:
    import protocol as p
    from google.protobuf.json_format import Parse
except ImportError:
    sys.exit("Error: This script requires you to run setup.sh")

from util import live_robot_only, vector_connection

def test_protocol_version(vector_connection):
    vector_connection.send("v1/protocol_version", p.ProtocolVersionRequest(), p.ProtocolVersionResponse())

def test_list_animations(vector_connection):
    vector_connection.send("v1/list_animations", p.ListAnimationsRequest(), p.ListAnimationsResponse())

def test_list_animation_triggers(vector_connection):
    vector_connection.send("v1/list_animation_triggers", p.ListAnimationTriggersRequest(), p.ListAnimationTriggersResponse())

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

def test_enable_marker_detection(vector_connection):
    vector_connection.send("v1/enable_marker_detection", p.EnableMarkerDetectionRequest(enable=True), p.EnableMarkerDetectionResponse())
    vector_connection.send("v1/enable_marker_detection", p.EnableMarkerDetectionRequest(enable=False), p.EnableMarkerDetectionResponse())

def test_enable_face_detection(vector_connection):
    vector_connection.send("v1/enable_face_detection", p.EnableFaceDetectionRequest(enable=True, enable_smile_detection=True, enable_expression_estimation=True, enable_blink_detection=True, enable_gaze_detection=True), p.EnableFaceDetectionResponse())
    vector_connection.send("v1/enable_face_detection", p.EnableFaceDetectionRequest(enable=False), p.EnableFaceDetectionResponse())
    
def test_enable_motion_detection(vector_connection):
    vector_connection.send("v1/enable_motion_detection", p.EnableMotionDetectionRequest(enable=True), p.EnableMotionDetectionResponse())
    vector_connection.send("v1/enable_motion_detection", p.EnableMotionDetectionRequest(enable=False), p.EnableMotionDetectionResponse())
    
def test_enable_mirror_mode(vector_connection):
    vector_connection.send("v1/enable_mirror_mode", p.EnableMirrorModeRequest(enable=True), p.EnableMirrorModeResponse())
    vector_connection.send("v1/enable_mirror_mode", p.EnableMirrorModeRequest(enable=False), p.EnableMirrorModeResponse())

# This can only run locally because webots will always have image streaming turned on (meaning the second rpc call will not succeed).
# Note: if there was a change made to have a webots world run without the keyboard controller enabled then this test could be re-enabled for local testing.
@live_robot_only
def test_enable_image_streaming(vector_connection):
    vector_connection.send("v1/enable_image_streaming", p.EnableImageStreamingRequest(enable=True), p.EnableImageStreamingResponse())
    vector_connection.send("v1/enable_image_streaming", p.EnableImageStreamingRequest(enable=False), p.EnableImageStreamingResponse())

def test_get_onboarding_state(vector_connection):
    vector_connection.send("v1/get_onboarding_state", p.OnboardingStateRequest(), p.OnboardingStateResponse())

def test_onboarding_complete_request(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_complete_request = p.OnboardingCompleteRequest() ),\
    p.OnboardingInputResponse())

def test_onboarding_wake_up_request(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_wake_up_request = p.OnboardingWakeUpRequest() ),\
    p.OnboardingInputResponse())

def test_onboarding_wake_up_started_request(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_wake_up_started_request = p.OnboardingWakeUpStartedRequest() ),\
    p.OnboardingInputResponse())

def test_skip_onboarding(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_skip_onboarding = p.OnboardingSkipOnboarding() ),\
    p.OnboardingInputResponse())

def test_restart_onboarding(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_restart = p.OnboardingRestart() ),\
    p.OnboardingInputResponse())

def test_set_onboarding_phase(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_set_phase_request = p.OnboardingSetPhaseRequest(phase=5) ),\
    p.OnboardingInputResponse())

def test_onboarding_phase_progress_request(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_phase_progress_request = p.OnboardingPhaseProgressRequest() ),\
    p.OnboardingInputResponse())

def test_onboarding_charge_info_request(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_charge_info_request = p.OnboardingChargeInfoRequest() ),\
    p.OnboardingInputResponse())

def test_onboarding_mark_complete_and_exit(vector_connection):
    vector_connection.send("v1/send_onboarding_input",\
    p.OnboardingInputRequest( onboarding_mark_complete_and_exit = p.OnboardingMarkCompleteAndExit() ),\
    p.OnboardingInputResponse())

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

@live_robot_only
def test_user_authentication(vector_connection):
    vector_connection.send("v1/user_authentication", p.UserAuthenticationRequest(), p.UserAuthenticationResponse())

def test_battery_state(vector_connection):
    vector_connection.send("v1/battery_state", p.BatteryStateRequest(), p.BatteryStateResponse())

def test_version_state(vector_connection):
    vector_connection.send("v1/version_state", p.VersionStateRequest(), p.VersionStateResponse())

# TODO Turn back on
# def test_connect_cube(vector_connection):
#     vector_connection.send("v1/connect_cube", p.ConnectCubeRequest(), p.ConnectCubeResponse())

# def test_disconnect_cube(vector_connection):
#     vector_connection.send("v1/disconnect_cube", p.DisconnectCubeRequest(), p.DisconnectCubeResponse())

# def test_cubes_available(vector_connection):
#     vector_connection.send("v1/cubes_available", p.CubesAvailableRequest(), p.CubesAvailableResponse())

# def test_flash_cube_lights(vector_connection):
#     vector_connection.send("v1/flash_cube_lights", p.FlashCubeLightsRequest(), p.FlashCubeLightsResponse())

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

@live_robot_only
def test_upload_debug_logs(vector_connection):
    def callback(response, response_type):
        print("Default response: {}".format(response.content))
        if response.status_code == 403:
            print("Rate limit for debug logs hit")
            return
        assert response.status_code == 200, "Received failure status_code: {} => {}".format(response.status_code, response.content)
        Parse(response.content, response_type, ignore_unknown_fields=True)
        print("Converted Protobuf: {}".format(response_type))
    vector_connection.send("v1/upload_debug_logs", p.UploadDebugLogsRequest(), p.UploadDebugLogsResponse(), callback=callback)

@live_robot_only
def test_check_cloud_connection(vector_connection):
    vector_connection.send("v1/check_cloud_connection", p.CheckCloudRequest(), p.CheckCloudResponse())

@pytest.mark.parametrize("data,result", [
    ('{"feature_name":"TestFeature"}',1),
    ('{"feature_name":"NotAFeature"}',0)
])
def test_feature_flag(vector_connection, data, result):
    def callback(response, response_type):
        print("Default response: {}".format(response.content))
        data = json.loads(response.content)
        assert "valid_feature" in data
        assert "feature_enabled" in data
        assert data["valid_feature"] == result
        assert data["feature_enabled"] == 0
    vector_connection.send_raw("v1/feature_flag", data, p.FeatureFlagResponse(), callback=callback)

@pytest.mark.parametrize("data,result", [
    ('{"request_list":[]}', 1),  # result is whether the result list is non-empty
    ('{"request_list":["TestFeature"]}', 0),
    ('{"request_list":["NotAFeature"]}', 0)
])
def test_feature_flag_list(vector_connection, data, result):
    length = 0
    def callback(response, response_type):
        print("Default response: {}".format(response.content))
        data = json.loads(response.content)
        assert "list" in data
        
        assert ((len(data["list"]) > 0) == result)
    vector_connection.send_raw("v1/feature_flag_list", data, p.FeatureFlagListResponse(), callback=callback)

def test_alexa_auth_state(vector_connection):
    def callback(response, response_type):
        print("Default response: {}".format(response.content))
    vector_connection.send("v1/alexa_auth_state", p.AlexaAuthStateRequest(), p.AlexaAuthStateResponse(), callback=callback)

@pytest.mark.parametrize("data", [
    '{"opt_in":true}'
])
def test_alexa_opt_in(vector_connection, data):
    vector_connection.send_raw("v1/alexa_opt_in", data, p.AlexaOptInResponse())

@pytest.mark.parametrize("data", [
    '{"opt_in":false}'
])
def test_alexa_opt_out(vector_connection, data):
    vector_connection.send_raw("v1/alexa_opt_in", data, p.AlexaOptInResponse())

def test_set_eye_color(vector_connection):
    vector_connection.send("v1/set_eye_color", p.SetEyeColorRequest(), p.SetEyeColorResponse())

# TODO Turn back on
#def test_capture_single_image(vector_connection):
#    vector_connection.send("v1/capture_single_image", p.CaptureSingleImageRequest(), p.CaptureSingleImageResponse())
