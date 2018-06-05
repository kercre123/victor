# Copyright (c) 2018 Anki, Inc.

'''
'''

__all__ = ['Robot', 'AsyncRobot']

import asyncio
import logging
import os
import sys

import aiogrpc
from . import util
from . import actions
from . import lights
from . import events
from .messaging import external_interface_pb2 as protocol
from .messaging import external_interface_pb2_grpc as client

module_logger = logging.getLogger(__name__)

class Robot:
    def __init__(self, ip, cert_file, port="443",
                 loop=None, is_async=False, default_logging=True):
        if cert_file is None:
            raise Exception("Must provide a cert file")
        with open(cert_file, 'rb') as cert:
            self.trusted_certs = cert.read()
        self.is_async = is_async
        if default_logging:
            util.setup_basic_logging()
        self.logger = util.get_class_logger(__name__, self)
        self.is_loop_owner = False
        if loop is None:
            self.logger.debug("Creating asyncio loop")
            self.is_loop_owner = True
            loop = asyncio.get_event_loop()
        self.loop = loop
        self.address = ':'.join([ip, port])
        self.channel = None
        self.connection = None
        self.events = events.EventHandler(self.loop)

    def connect(self):
        credentials = aiogrpc.ssl_channel_credentials(root_certificates=self.trusted_certs)
        self.logger.info("Connecting to {}".format(self.address))
        self.channel = aiogrpc.secure_channel(self.address, credentials)
        self.connection = client.ExternalInterfaceStub(self.channel)
        self.events.start(self.connection)

    def disconnect(self, wait_for_tasks=True):
        if self.is_async and wait_for_tasks:
            for task in self.pending:
                task.wait_for_completed()
        self.events.close()
        if self.channel:
            self.loop.run_until_complete(self.channel.close())
        if self.is_loop_owner:
            self.loop.close()

    def __enter__(self):
        self.connect()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.disconnect()

    # Animations
    async def get_anim_names(self, enable_diagnostics=False):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.RequestAvailableAnimations()
        innerWrappedMessage = _clad_message.Animations(RequestAvailableAnimations=message)
        outerWrappedMessage = _clad_message.ExternalComms(Animations=innerWrappedMessage)
        await self.socket.send(outerWrappedMessage.pack())
        return await self.collect_events(enable_diagnostics, 10)

    @actions._as_actionable
    async def play_anim(self, animation_name, loop_count=1, ignore_body_track=True, ignore_head_track=True, ignore_lift_track=True):
        anim = protocol.Animation(name=animation_name)
        req = protocol.PlayAnimationRequest(animation=anim, loops=loop_count) # TODO: add body tracks
        result = await self.connection.PlayAnimation(req)
        self.logger.debug(result)
        return result

    #TODO Refactor out of robot.py
    async def __read_file_in_chunks(self, file_path, chunk_size):
        if not os.path.isfile(file_path):
            raise ValueError("File missing: %s" % file_path)
        chunks = []
        with open(file_path, 'rb') as fh:
            chunk = fh.read(chunk_size)
            while chunk:
                chunks.append(chunk)
                chunk = fh.read(chunk_size)
        return chunks

    # #TODO Refactor out of robot.py
    # class FileType(IntEnum):
    #     Animation = 0
    #     FaceImg = 1

    #TODO Refactor out of robot.py
    MAX_MSG_SIZE = 2048
    TRANSFER_FILE_MSG_OVERHEAD = 6
    CLAD_MSG_OVERHEAD = 15
    async def transfer_file(self, file_type, anim_file_path, enable_diagnostics=False):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        file_chunk_size = self.MAX_MSG_SIZE - self.TRANSFER_FILE_MSG_OVERHEAD - len(os.path.basename(anim_file_path)) - self.CLAD_MSG_OVERHEAD
        chunks = await self.__read_file_in_chunks(anim_file_path, file_chunk_size)
        num_chunks = len(chunks)
        if enable_diagnostics==True:
            self.logger.debug("Transfering file %s" % (anim_file_path)),
            self.logger.debug("Transfering %d chunks" % (num_chunks)),
        for idx in range(num_chunks):
            message = _clad_message.TransferFile(fileBytes=chunks[idx], filePart=idx,
                      numFileParts=num_chunks, filename=os.path.basename(anim_file_path),
                      fileType=file_type)
            innerWrappedMessage = _clad_message.Animations(TransferFile=message)
            outerWrappedMessage = _clad_message.ExternalComms(Animations=innerWrappedMessage)
            await self.socket.send(outerWrappedMessage.pack())
            if enable_diagnostics==True:
                logger.debug("Transfered chunk %d" % idx),

    # Low level motor functions
    @actions._as_actionable
    async def set_wheel_motors(self, left_wheel_speed, right_wheel_speed, left_wheel_accel=0.0, right_wheel_accel=0.0):
        motors = protocol.DriveWheelsRequest(left_wheel_mmps=left_wheel_speed,
                                             right_wheel_mmps=right_wheel_speed,
                                             left_wheel_mmps2=left_wheel_accel,
                                             right_wheel_mmps2=right_wheel_accel)
        result = await self.connection.DriveWheels(motors)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_wheel_motors_turn(self, turn_speed, turn_accel=0.0, curvatureRadius_mm=0):
        drive_arc_request = protocol.DriveArcRequest(speed=turn_speed, accel=turn_accel, curvatureRadius_mm=1)
        result = await self.connection.DriveArc(drive_arc_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_head_motor(self, speed):
        set_head_request = protocol.MoveHeadRequest(speed_rad_per_sec=speed)
        result = await self.connection.MoveHead(set_head_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    @actions._as_actionable
    async def set_lift_motor(self, speed):
        set_lift_request = protocol.MoveLiftRequest(speed_rad_per_sec=speed)
        result = await self.connection.MoveLift(set_lift_request)
        self.logger.info(f'{type(result)}: {str(result).strip()}')
        return result

    async def meet_victor(self, name):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.AppIntent('intent_meet_victor', name)
        innerWrappedMessage = _clad_message.MeetVictor(AppIntent=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def cancel_face_enrollment(self):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.CancelFaceEnrollment()
        innerWrappedMessage = _clad_message.MeetVictor(CancelFaceEnrollment=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def request_enrolled_names(self):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
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
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.UpdateEnrolledFaceByID(face_id, old_name, new_name)
        innerWrappedMessage = _clad_message.MeetVictor(UpdateEnrolledFaceByID=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def erase_enrolled_face_by_id(self, face_id):
        '''Erase the enrollment (name) record for the face with this ID.

        Args:
            face_id (int): The ID of the face to erase.
        '''
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.EraseEnrolledFaceByID(face_id)
        innerWrappedMessage = _clad_message.MeetVictor(EraseEnrolledFaceByID=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def erase_all_enrolled_faces(self):
        '''Erase the enrollment (name) records for all faces.
        '''
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.EraseAllEnrolledFaces()
        innerWrappedMessage = _clad_message.MeetVictor(EraseAllEnrolledFaces=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_face_to_enroll(self, name, observedID=0, saveID=0, saveToRobot=True, sayName=False, useMusic=False):
        sys.exit("'{}' is not yet implemented in grpc".format(__name__))
        message = _clad_message.SetFaceToEnroll(name, observedID, saveID, saveToRobot, sayName, useMusic)
        innerWrappedMessage = _clad_message.MeetVictor(SetFaceToEnroll=message)
        outerWrappedMessage = _clad_message.ExternalComms(MeetVictor=innerWrappedMessage)
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

    # Vector Display
    async def set_backpack_lights(self, light1, light2, light3, backpack_color_profile=lights.white_balanced_backpack_profile):
        '''Set the lights on Vector's backpack.

        The light descriptions below are all from Vector's perspective.

        Args:
            light1 (:class:`cozmo.lights.Light`): The front backpack light
            light2 (:class:`cozmo.lights.Light`): The center backpack light
            light3 (:class:`cozmo.lights.Light`): The rear backpack light
        '''
        message = _clad_message.SetBackpackLEDs()
        for i, light in enumerate( (light1, light2, light3) ):
            if light is not None:
                lights._set_light(message, i, light, backpack_color_profile)

        innerWrappedMessage = _clad_message.VictorDisplay(SetBackpackLEDs=message)
        outerWrappedMessage = _clad_message.ExternalComms(VictorDisplay=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_all_backpack_lights(self, light, color_profile=lights.white_balanced_backpack_profile):
        '''Set the lights on Vector's backpack to the same color.

        Args:
            light (:class:`cozmo.lights.Light`): The lights for Vector's backpack.
        '''
        light_arr = [ light ] * 3
        await self.set_backpack_lights(*light_arr, color_profile)

    async def set_cube_light_corners( self, cube_id, light1, light2, light3, light4, color_profile=lights.white_balanced_cube_profile ):
        message = _clad_message.SetAllActiveObjectLEDs(
            objectID=cube_id,
            offset=[0,0,0,0])
        for i, light in enumerate( (light1, light2, light3, light4) ):
            if light is not None:
                lights._set_light(message, i, light, color_profile)

        innerWrappedMessage = _clad_message.Cubes(SetAllActiveObjectLEDs=message)
        outerWrappedMessage = _clad_message.ExternalComms(Cubes=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())

    async def set_cube_lights( self, cube_id, light, color_profile=lights.white_balanced_cube_profile ):
        await self.set_cube_light_corners( cube_id, *[light, light, light, light], color_profile)

    async def set_oled_to_color(self, color, duration_sec, interrupt_running=True):
        message = _clad_message.DisplayFaceImageRGB()
        message.duration_ms = int(1000 * duration_sec)

        rgb565 = [
            ((color.int_color >> 24) & 0xff) >> 3,
            ((color.int_color >> 16) & 0xff) >> 2,
            ((color.int_color >> 8) & 0xff) >> 3
            ]

        int_565_color = (rgb565[0]<<11) | (rgb565[1] << 5) | rgb565[2]

        message.faceData = [int_565_color] * 17664
        message.interruptRunning = interrupt_running

        innerWrappedMessage = _clad_message.VictorDisplay(DisplayFaceImageRGB=message)
        outerWrappedMessage = _clad_message.ExternalComms(VictorDisplay=innerWrappedMessage)

        await self.socket.send(outerWrappedMessage.pack())


class AsyncRobot(Robot):
    def __init__(self, *args, **kwargs):
        super(AsyncRobot, self).__init__(*args, **kwargs, is_async=True)
        self.pending = []

    def add_pending(self, task):
        self.pending += [task]

    def remove_pending(self, task):
        self.pending = [x for x in self.pending if x is not task]
