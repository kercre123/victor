#!/usr/bin/env python3

import argparse
import asyncio
import math
import struct

import websockets

from ._clad import _clad_message

class Robot:
    def __init__(self, socket):
        self.socket = socket

    # Animations
    async def get_anim_names(self):
        message = _clad_message.RequestAvailableAnimations()
        innerWrappedMessage = _clad_message.Animations(requestAvailableAnimations=message)
        outerWrappedMessage = _clad_message.ExternalComms(Animations=innerWrappedMessage)
        await self.socket.send(outerWrappedMessage.pack()) 
        # TODO (VIC-2784): wait for the list...d
        return []

    async def play_anim(self, animation_name, loop_count=1, ignore_body_track = True, ignore_head_track = True, ignore_lift_track = True ):
        message = _clad_message.PlayAnimation(
            animationName = animation_name, numLoops = loop_count, ignoreBodyTrack = ignore_body_track, ignoreHeadTrack = ignore_head_track, ignoreLiftTrack = ignore_lift_track)
        innerWrappedMessage = _clad_message.Animations(playAnimation=message)
        outerWrappedMessage = _clad_message.ExternalComms(Animations=innerWrappedMessage)
        await self.socket.send(outerWrappedMessage.pack()) 

    # Low level motor functions
    async def set_wheel_motors(self, left_wheel_speed, right_wheel_speed, left_wheel_accel=0.0, right_wheel_accel=0.0):
        message = _clad_message.DriveWheels(
            lwheel_speed_mmps=left_wheel_speed,
            rwheel_speed_mmps=right_wheel_speed,
            lwheel_accel_mmps2=left_wheel_accel,
            rwheel_accel_mmps2=right_wheel_accel)
        innerWrappedMessage = _clad_message.MotorControl(DriveWheels=message)
        outerWrappedMessage = _clad_message.ExternalComms(MotorControl=innerWrappedMessage)
        await self.socket.send(outerWrappedMessage.pack())

    async def set_wheel_motors_turn(self, turn_speed, turn_accel=0.0):
        message = _clad_message.DriveArc(speed=turn_speed, accel=turn_accel)
        innerWrappedMessage = _clad_message.MotorControl(DriveArc=message)
        outerWrappedMessage = _clad_message.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_head_motor(self, speed):
        message = _clad_message.MoveHead(speed_rad_per_sec=speed)
        innerWrappedMessage = _clad_message.MotorControl(MoveHead=message)
        outerWrappedMessage = _clad_message.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_lift_motor(self, speed):
        message = _clad_message.MoveLift(speed_rad_per_sec=speed)
        innerWrappedMessage = _clad_message.MotorControl(MoveLift=message)
        outerWrappedMessage = _clad_message.ExternalComms(MotorControl=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def meet_victor(self, name):
        message = _clad_message.AppIntent('intent_meet_victor', name)
        innerWrappedMessage = _clad_message.MeetVictor(AppIntent=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def cancel_face_enrollment(self):
        message = _clad_message.CancelFaceEnrollment()
        innerWrappedMessage = _clad_message.MeetVictor(CancelFaceEnrollment=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def request_enrolled_names(self):
        message = _clad_message.RequestEnrolledNames()
        innerWrappedMessage = _clad_message.MeetVictor(RequestEnrolledNames=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def update_enrolled_face_by_id(self, face_id, old_name, new_name):
        '''Update the name enrolled for a given face.

        Args:
            face_id (int): The ID of the face to rename.
            old_name (string): The old name of the face (must be correct, otherwise message is ignored).
            new_name (string): The new name for the face.
        '''        
        message = _clad_message.UpdateEnrolledFaceByID(face_id, old_name, new_name)
        innerWrappedMessage = _clad_message.MeetVictor(UpdateEnrolledFaceByID=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def erase_enrolled_face_by_id(self, face_id):
        '''Erase the enrollment (name) record for the face with this ID.

        Args:
            face_id (int): The ID of the face to erase.
        '''        
        message = _clad_message.EraseEnrolledFaceByID(face_id)
        innerWrappedMessage = _clad_message.MeetVictor(EraseEnrolledFaceByID=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def erase_all_enrolled_faces(self):
        '''Erase the enrollment (name) records for all faces.
        '''        
        message = _clad_message.EraseAllEnrolledFaces()
        innerWrappedMessage = _clad_message.MeetVictor(EraseAllEnrolledFaces=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_face_to_enroll(self, name, observedID=0, saveID=0, saveToRobot=True, sayName=False, useMusic=False):
        message = _clad_message.SetFaceToEnroll(name, observedID, saveID, saveToRobot, sayName, useMusic)
        innerWrappedMessage = _clad_message.MeetVictor(SetFaceToEnroll=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)
        
        await self.socket.send(outerWrappedMessage.pack())        

    # Mid level movement actions
    async def drive_off_contacts(self):
        message = _clad_message.DriveOffChargerContacts()
        innerWrappedMessage = _clad_message.MovementAction(DriveOffChargerContacts=message)
        outerWrappedMessage = _clad_message.ExternalComms(MovementAction=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())
    
    async def drive_straight(self, distance_mm, speed_mmps=100.0, play_animation=True):
        '''Tells Vector to drive in a straight line

        Vector will drive for the specified distance (forwards or backwards)
        '''
        message = _clad_message.DriveStraight(
            speed_mmps=speed_mmps,
            dist_mm=distance_mm,
            shouldPlayAnimation=play_animation)
        innerWrappedMessage = _clad_message.MovementAction(DriveStraight=message)
        outerWrappedMessage = _clad_message.ExternalComms(MovementAction=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def turn_in_place(self, angle_rad, speed_radps=math.pi, accel_radps2=0.0):
        '''Turn the robot around its current position.
        '''
        message = _clad_message.TurnInPlace(
            angle_rad=angle_rad,
            speed_rad_per_sec=speed_radps,
            accel_rad_per_sec2=accel_radps2)
        innerWrappedMessage = _clad_message.MovementAction(TurnInPlace=message)
        outerWrappedMessage = _clad_message.ExternalComms(MovementAction=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_head_angle(self, angle_rad, max_speed_radps=0.0, accel_radps2=0.0, duration_sec=0.0):
        message = _clad_message.SetHeadAngle(
            angle_rad=angle_rad,
            max_speed_rad_per_sec=max_speed_radps,
            accel_rad_per_sec2=accel_radps2,
            duration_sec=duration_sec)
        innerWrappedMessage = _clad_message.MovementAction(SetHeadAngle=message)
        outerWrappedMessage = _clad_message.ExternalComms(MovementAction=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_lift_height(self, height_mm, max_speed_radps=0.0, accel_radps2=0.0, duration_sec=0.0):
        message = _clad_message.SetLiftHeight(
            height_mm=height_mm,
            max_speed_rad_per_sec=max_speed_radps,
            accel_rad_per_sec2=accel_radps2,
            duration_sec=duration_sec)
        innerWrappedMessage = _clad_message.MovementAction(SetLiftHeight=message)
        outerWrappedMessage = _clad_message.ExternalComms(MovementAction=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

async def _bootstrap(main_function, uri):
    print("Attempting websockets.connect...")
    async with websockets.connect(uri) as websocket:
        print("websockets.connect success")
        robot = Robot(websocket)
        await main_function(robot)

def run_program(main_function, ip):
    if ip is None:
        print('run_program requires an ip address')
    else:
        connection_uri = 'ws://' + ip
        try:
            loop = asyncio.get_event_loop()
            loop.run_until_complete(_bootstrap(main_function, connection_uri))
        finally:
            loop.close()
