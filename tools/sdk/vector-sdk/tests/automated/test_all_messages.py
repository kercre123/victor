#!/usr/bin/env python3

'''
Calls specific messages on the robot, with expected results and verifies that the robot's responses match up
 - Exceptions will be raised if a response is of the wrong type, or has the wrong data
 - Exceptions will be raised if the interface defines a message that is neither on the test list or ignore list
'''

import argparse
import asyncio
import os
from pathlib import Path
import sys
import time

sys.path.append(os.path.join(os.path.dirname(__file__), '../..'))
import vector

from vector.messaging import external_interface_pb2
from vector.messaging import protocol
from vector.messaging import client

# Both the EventStream and RobotStream should be kicked off automatically when we initialize a connection.
# while I feel it could be useful to verify these events come from the robot in response to the correct 
# stimula and that the robot state correctly represents its actual state, testing the scope of these two
# messages feels different from the other direct send->response cases and as it will probably require 
# webots state management feels improper for a rapid smoke test.
#  - nicolas 06/18/18
messages_to_ignore = [
    client.ExternalInterfaceServicer.EventStream,
    client.ExternalInterfaceServicer.RobotStateStream
    ]

messages_to_test = [
    # DriveWheels message
    ( client.ExternalInterfaceServicer.DriveWheels, 
        protocol.DriveWheelsRequest(left_wheel_mmps=0.0, right_wheel_mmps=0.0, left_wheel_mmps2=0.0, right_wheel_mmps2=0.0), 
        protocol.DriveWheelsResult(status=protocol.ResultStatus(description="Message sent to engine")) ),

    # DriveArc message
    ( client.ExternalInterfaceServicer.DriveArc,
        protocol.DriveArcRequest(speed=0.0, accel=0.0, curvatureRadius_mm=0),
        protocol.DriveArcResult(status=protocol.ResultStatus(description="Message sent to engine")) ),

    # MoveHead message
    ( client.ExternalInterfaceServicer.MoveHead, 
        protocol.MoveHeadRequest(speed_rad_per_sec=0.0),
        protocol.MoveHeadResult(status=protocol.ResultStatus(description="Message sent to engine")) ),

    # MoveLift message
    ( client.ExternalInterfaceServicer.MoveLift,
        protocol.MoveLiftRequest(speed_rad_per_sec=0.0),
        protocol.MoveLiftResult(status=protocol.ResultStatus(description="Message sent to engine")) ),

    # PlayAnimation message
    ( client.ExternalInterfaceServicer.PlayAnimation, 
        protocol.PlayAnimationRequest(animation=protocol.Animation(name='anim_poked_giggle'), loops=1), 
        protocol.PlayAnimationResult(status=protocol.ResultStatus(description="Animation completed")) ),

    # DisplayFaceImageRGB message
    ( client.ExternalInterfaceServicer.DisplayFaceImageRGB, 
        protocol.DisplayFaceImageRGBRequest(face_data=bytes(vector.color.Color(rgb=[255,0,0]).rgb565_bytepair * 17664), duration_ms=1000, interrupt_running=True), 
        protocol.DisplayFaceImageRGBResult(status=protocol.ResultStatus(description="Message sent to engine")) ),

    # SDKBehaviorActivation message
    ( client.ExternalInterfaceServicer.SDKBehaviorActivation,
        protocol.SDKActivationRequest(slot=vector.robot.SDK_PRIORITY_LEVEL.SDK_HIGH_PRIORITY, enable=True), 
        protocol.SDKActivationResult(status=protocol.ResultStatus(description="SDKActivationResult returned"), slot=vector.robot.SDK_PRIORITY_LEVEL.SDK_HIGH_PRIORITY, enabled=True)),

    # AppIntent message
    ( client.ExternalInterfaceServicer.AppIntent,
        protocol.AppIntentRequest(intent='intent_meet_victor', param='Bobert'), 
        protocol.AppIntentResult(status=protocol.ResultStatus(description="Message sent to engine"))),

    # UpdateEnrolledFaceByID message
    ( client.ExternalInterfaceServicer.UpdateEnrolledFaceByID,
        protocol.UpdateEnrolledFaceByIDRequest(face_id=1, old_name="Bobert", new_name="Boberta"),
        protocol.UpdateEnrolledFaceByIDResult(status=protocol.ResultStatus(description="Message sent to engine"))),

    # SetFaceToEnroll message
    ( client.ExternalInterfaceServicer.SetFaceToEnroll,
        protocol.SetFaceToEnrollRequest(name="Boberta", observed_id=1, save_id=0, save_to_robot=True, say_name=True, use_music=True), 
        protocol.SetFaceToEnrollResult(status=protocol.ResultStatus(description="Message sent to engine"))),    

    # CancelFaceEnrollment message
    ( client.ExternalInterfaceServicer.CancelFaceEnrollment,
        protocol.CancelFaceEnrollmentRequest(), 
        protocol.CancelFaceEnrollmentResult(status=protocol.ResultStatus(description="Message sent to engine"))),

    # EraseEnrolledFaceByID message
    ( client.ExternalInterfaceServicer.EraseEnrolledFaceByID,
        protocol.EraseEnrolledFaceByIDRequest(face_id=1), 
        protocol.EraseEnrolledFaceByIDResult(status=protocol.ResultStatus(description="Message sent to engine"))),

    # EraseAllEnrolledFaces message
    ( client.ExternalInterfaceServicer.EraseAllEnrolledFaces,
        protocol.EraseAllEnrolledFacesRequest(), 
        protocol.EraseAllEnrolledFacesResult(status=protocol.ResultStatus(description="Message sent to engine"))),

    # RequestEnrolledNames message
    ( client.ExternalInterfaceServicer.RequestEnrolledNames,
        protocol.RequestEnrolledNamesRequest(),
        protocol.RequestEnrolledNamesResult(status=protocol.ResultStatus(description="Enrolled names returned"), faces=[])),

    # NOTE: Add additional messages here
    ]

async def test_message(robot, message_name, message_src, message_input, expected_message_output, errors):
    # The message_src is used mostly so we can easily verify that the name is supported by the servicer.  In terms of actually making the call its simpler to invoke on the robot
    message_call = getattr(robot.connection, message_name)
    result = await message_call(message_input)
    
    expected_result_data_fields = [a[1] for a in expected_message_output.ListFields()]
    result_data_fields = [a[1] for a in result.ListFields()]

    # Casting as string makes the equality check work
    if str(type(result)) != str(type(expected_message_output)):
        errors.append('TypeError: recieved output of type {0} when expecting output of type {1} for message of type {2}'.format(type(result), type(expected_message_output), message_name))
        return

    if len(expected_result_data_fields) != len(result_data_fields):
        errors.append('TypeError: recieved output that appears to be a different type or contains different contents {0} than the expected output type {1} for message of type {2}'.format(type(result), type(expected_message_output), message_name))
        return

    # This does not perform a deep comparison, which is difficult to implement in a generic way
    for i in range(len(expected_result_data_fields)):
        # @TODO: Need a way to check if expected_result_data_fields[i] is expecting a wildcard
        if result_data_fields[i] != expected_result_data_fields[i]:
            errors.append('ValueError: recieved output with incorrect response {0} for message of type {1}, was expecting {2}, failure occurred with field "{3}"'.format(str(result_data_fields), message_name, str(expected_result_data_fields), str(result_data_fields[i])))

async def run_message_tests(robot, future):
    errors = []

    # compile a list of all functions in the interface and the input/output classes we expect them to utilize
    all_methods_in_interface = external_interface_pb2.DESCRIPTOR.services_by_name['ExternalInterface'].methods
    expected_test_list = {}
    for i in all_methods_in_interface:
        expected_test_list[i.name] = {
            'input': i.input_type,
            'output': i.output_type,
            }

    # strip out any messages that we're explicitly ignoring
    for i in messages_to_ignore:
        message_name = i.__name__
        del expected_test_list[message_name]

    # run through all listed test cases
    for i in messages_to_test:
        message_call = i[0]
        input_data = i[1]
        output_data = i[2]

        message_name = message_call.__name__
        # make sure we are expecting this message
        if message_name in expected_test_list:
            # make sure we are using the correct input class for this message
            expected_input_type = expected_test_list[message_name]['input'].name
            recieved_input_type = type(input_data).__name__
            if recieved_input_type != expected_input_type:
                errors.append('InputData: A test for a message of type {0} expects input data of the type {1}, but {2} was supplied'.format(message_name, expected_input_type, recieved_input_type))
                continue

            # make sure we are using the correct output class for this message
            expected_output_type = expected_test_list[message_name]['output'].name
            recieved_output_type = type(output_data).__name__
            if recieved_output_type != expected_output_type:
                errors.append('OutputData: A test for a message of type {0} expects output data of the type {1}, but {2} was supplied'.format(message_name, expected_output_type, recieved_output_type))
                continue

            print("testing " + message_name)
            await test_message(robot, message_name, message_call, input_data, output_data, errors)
            del expected_test_list[message_name]
        else:
            errors.append('NotImplemented: A test was defined for the {0} message, which is not in the interface'.format(message_name))

    # squawk if we missed any messages in the inteface
    if len(expected_test_list) > 0:
        errors.append('NotImplemented: The following messages exist in the interface and do not have a corresponding test: {0}'.format(str(expected_test_list)))

    future.set_result(errors)

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("ip")
    parser.add_argument("cert_file")
    args = parser.parse_args()

    cert = Path(args.cert_file)
    cert.resolve()

    with vector.Robot(args.ip, str(cert)) as robot:
        print("------ beginning tests ------")

        loop = robot.loop
        future = asyncio.Future()
        robot.loop.run_until_complete(run_message_tests(robot, future))

        errors = future.result()
        if len(errors) == 0:
            print("------ all tests finished successfully! ------")
        else:
            print("------ tests finished with {0} errors! ------".format(len(errors)))
            for a in errors:
                print(a)
        print('\n')

if __name__ == "__main__":
    main()
