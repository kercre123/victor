












using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;

using s8 = System.SByte;
using s16 = System.Int16;
using s32 = System.Int32;
using s64 = System.Int64;
using u8 = System.Byte;
using u16 = System.UInt16;
using u32 = System.UInt32;
using u64 = System.UInt64;
using f32 = System.Single;
using f64 = System.Double;

public enum NetworkMessageID {
  NoMessage = 0,






U2G_ConnectToRobot,




U2G_ConnectToUiDevice,




U2G_DisconnectFromUiDevice,





U2G_ForceAddRobot,







U2G_StartEngine,





U2G_DriveWheels,





U2G_TurnInPlace,





U2G_MoveHead,




U2G_MoveLift,




U2G_SetLiftHeight,






U2G_SetHeadAngle,






U2G_StopAllMotors,



U2G_ImageRequest,
U2G_SetRobotImageSendMode,





U2G_SaveImages,




U2G_EnableDisplay,




U2G_SetHeadlights,




U2G_GotoPose,







U2G_PlaceObjectOnGround,







U2G_PlaceObjectOnGroundHere,



U2G_ExecuteTestPlan,



U2G_SelectNextObject,



U2G_PickAndPlaceObject,





U2G_TraverseObject,



U2G_ClearAllBlocks,



U2G_ExecuteBehavior,




U2G_SetBehaviorState,




U2G_AbortPath,



U2G_AbortAll,



U2G_DrawPoseMarker,







U2G_ErasePoseMarker,



U2G_SetHeadControllerGains,






U2G_SetLiftControllerGains,






U2G_SelectNextSoundScheme,



U2G_StartTestMode,







U2G_IMURequest,




U2G_PlayAnimation,






U2G_ReadAnimationFile,



U2G_StartFaceTracking,





U2G_StopFaceTracking,



U2G_StartLookingForMarkers,



U2G_StopLookingForMarkers,



U2G_SetVisionSystemParams,
U2G_SetFaceDetectParams,




G2U_RobotAvailable,



G2U_UiDeviceAvailable,



G2U_RobotConnected,




G2U_UiDeviceConnected,
G2U_RobotState,
G2U_ImageChunk,
G2U_RobotObservedObject,
G2U_DeviceDetectedVisionMarker,
G2U_PlaySound,





G2U_StopSound,
}







public partial class U2G_ConnectToRobot : NetworkMessage {
public u8 robotID;
public override int ID { get { return (int)NetworkMessageID.U2G_ConnectToRobot; } } }


public partial class U2G_ConnectToUiDevice : NetworkMessage {
public u8 deviceID;
public override int ID { get { return (int)NetworkMessageID.U2G_ConnectToUiDevice; } } }


public partial class U2G_DisconnectFromUiDevice : NetworkMessage {
public u8 deviceID;
public override int ID { get { return (int)NetworkMessageID.U2G_DisconnectFromUiDevice; } } }



public partial class U2G_ForceAddRobot : NetworkMessage {
public u8[] ipAddress = new u8[16];
public u8 robotID;
public u8 isSimulated;
public override int ID { get { return (int)NetworkMessageID.U2G_ForceAddRobot; } } }



public partial class U2G_StartEngine : NetworkMessage {
public u8 asHost;
public u8[] vizHostIP = new u8[16];
public override int ID { get { return (int)NetworkMessageID.U2G_StartEngine; } } }


public partial class U2G_DriveWheels : NetworkMessage {
public f32 lwheel_speed_mmps;
public f32 rwheel_speed_mmps;
public override int ID { get { return (int)NetworkMessageID.U2G_DriveWheels; } } }


public partial class U2G_TurnInPlace : NetworkMessage {
public f32 angle_rad;
public u8 robotID;
public override int ID { get { return (int)NetworkMessageID.U2G_TurnInPlace; } } }


public partial class U2G_MoveHead : NetworkMessage {
public f32 speed_rad_per_sec;
public override int ID { get { return (int)NetworkMessageID.U2G_MoveHead; } } }


public partial class U2G_MoveLift : NetworkMessage {
public f32 speed_rad_per_sec;
public override int ID { get { return (int)NetworkMessageID.U2G_MoveLift; } } }


public partial class U2G_SetLiftHeight : NetworkMessage {
public f32 height_mm;
public f32 max_speed_rad_per_sec;
public f32 accel_rad_per_sec2;
public override int ID { get { return (int)NetworkMessageID.U2G_SetLiftHeight; } } }


public partial class U2G_SetHeadAngle : NetworkMessage {
public f32 angle_rad;
public f32 max_speed_rad_per_sec;
public f32 accel_rad_per_sec2;
public override int ID { get { return (int)NetworkMessageID.U2G_SetHeadAngle; } } }


public partial class U2G_StopAllMotors : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_StopAllMotors; } } }


public partial class U2G_ImageRequest : NetworkMessage {
public u8 robotID;
public u8 mode;
public override int ID { get { return (int)NetworkMessageID.U2G_ImageRequest; } } }





public partial class U2G_SetRobotImageSendMode : NetworkMessage {
public u8 mode;
public u8 resolution;
public override int ID { get { return (int)NetworkMessageID.U2G_SetRobotImageSendMode; } } }


public partial class U2G_SaveImages : NetworkMessage {
public u8 enableSave;
public override int ID { get { return (int)NetworkMessageID.U2G_SaveImages; } } }


public partial class U2G_EnableDisplay : NetworkMessage {
public u8 enable;
public override int ID { get { return (int)NetworkMessageID.U2G_EnableDisplay; } } }


public partial class U2G_SetHeadlights : NetworkMessage {
public u8 intensity;
public override int ID { get { return (int)NetworkMessageID.U2G_SetHeadlights; } } }


public partial class U2G_GotoPose : NetworkMessage {
public f32 x_mm;
public f32 y_mm;
public f32 rad;
public u8 level;
public override int ID { get { return (int)NetworkMessageID.U2G_GotoPose; } } }


public partial class U2G_PlaceObjectOnGround : NetworkMessage {
public f32 x_mm;
public f32 y_mm;
public f32 rad;
public u8 level;
public override int ID { get { return (int)NetworkMessageID.U2G_PlaceObjectOnGround; } } }


public partial class U2G_PlaceObjectOnGroundHere : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_PlaceObjectOnGroundHere; } } }


public partial class U2G_ExecuteTestPlan : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_ExecuteTestPlan; } } }


public partial class U2G_SelectNextObject : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_SelectNextObject; } } }


public partial class U2G_PickAndPlaceObject : NetworkMessage {
public s32 objectID;
public u8 usePreDockPose;
public override int ID { get { return (int)NetworkMessageID.U2G_PickAndPlaceObject; } } }


public partial class U2G_TraverseObject : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_TraverseObject; } } }


public partial class U2G_ClearAllBlocks : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_ClearAllBlocks; } } }


public partial class U2G_ExecuteBehavior : NetworkMessage {
public u8 behaviorMode;
public override int ID { get { return (int)NetworkMessageID.U2G_ExecuteBehavior; } } }


public partial class U2G_SetBehaviorState : NetworkMessage {
public u8 behaviorState;
public override int ID { get { return (int)NetworkMessageID.U2G_SetBehaviorState; } } }


public partial class U2G_AbortPath : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_AbortPath; } } }


public partial class U2G_AbortAll : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_AbortAll; } } }


public partial class U2G_DrawPoseMarker : NetworkMessage {
public f32 x_mm;
public f32 y_mm;
public f32 rad;
public u8 level;
public override int ID { get { return (int)NetworkMessageID.U2G_DrawPoseMarker; } } }


public partial class U2G_ErasePoseMarker : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_ErasePoseMarker; } } }


public partial class U2G_SetHeadControllerGains : NetworkMessage {
public f32 kp;
public f32 ki;
public f32 maxIntegralError;
public override int ID { get { return (int)NetworkMessageID.U2G_SetHeadControllerGains; } } }


public partial class U2G_SetLiftControllerGains : NetworkMessage {
public f32 kp;
public f32 ki;
public f32 maxIntegralError;
public override int ID { get { return (int)NetworkMessageID.U2G_SetLiftControllerGains; } } }


public partial class U2G_SelectNextSoundScheme : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_SelectNextSoundScheme; } } }


public partial class U2G_StartTestMode : NetworkMessage {
public s32 p1;
public s32 p2;
public s32 p3;
public u8 mode;
public override int ID { get { return (int)NetworkMessageID.U2G_StartTestMode; } } }


public partial class U2G_IMURequest : NetworkMessage {
public u32 length_ms;
public override int ID { get { return (int)NetworkMessageID.U2G_IMURequest; } } }


public partial class U2G_PlayAnimation : NetworkMessage {
public u32 numLoops;

public char[] animationName = new char[32];
public override int ID { get { return (int)NetworkMessageID.U2G_PlayAnimation; } } }


public partial class U2G_ReadAnimationFile : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_ReadAnimationFile; } } }


public partial class U2G_StartFaceTracking : NetworkMessage {

public u8 timeout_sec;
public override int ID { get { return (int)NetworkMessageID.U2G_StartFaceTracking; } } }


public partial class U2G_StopFaceTracking : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_StopFaceTracking; } } }


public partial class U2G_StartLookingForMarkers : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_StartLookingForMarkers; } } }


public partial class U2G_StopLookingForMarkers : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.U2G_StopLookingForMarkers; } } }


public partial class U2G_SetVisionSystemParams : NetworkMessage {
public s32 autoexposureOn;
public f32 exposureTime;
public s32 integerCountsIncrement;
public f32 minExposureTime;
public f32 maxExposureTime;
public f32 percentileToMakeHigh;
public s32 limitFramerate;
public u8 highValue;
public override int ID { get { return (int)NetworkMessageID.U2G_SetVisionSystemParams; } } }


public partial class U2G_SetFaceDetectParams : NetworkMessage {
public f32 scaleFactor;
public s32 minNeighbors;
public s32 minObjectHeight;
public s32 minObjectWidth;
public s32 maxObjectHeight;
public s32 maxObjectWidth;
public override int ID { get { return (int)NetworkMessageID.U2G_SetFaceDetectParams; } } }




public partial class G2U_RobotAvailable : NetworkMessage {
public u32 robotID;
public override int ID { get { return (int)NetworkMessageID.G2U_RobotAvailable; } } }

public partial class G2U_UiDeviceAvailable : NetworkMessage {
public u32 deviceID;
public override int ID { get { return (int)NetworkMessageID.G2U_UiDeviceAvailable; } } }

public partial class G2U_RobotConnected : NetworkMessage {
public u32 robotID;
public u8 successful;
public override int ID { get { return (int)NetworkMessageID.G2U_RobotConnected; } } }

public partial class G2U_UiDeviceConnected : NetworkMessage {
public u32 deviceID;
public u8 successful;
public override int ID { get { return (int)NetworkMessageID.G2U_UiDeviceConnected; } } }
public partial class G2U_RobotState : NetworkMessage {
public f32 pose_x;
public f32 pose_y;
public f32 pose_z;
public f32 poseAngle_rad;
public f32 leftWheelSpeed_mmps;
public f32 rightWheelSpeed_mmps;
public f32 headAngle_rad;
public f32 liftHeight_mm;
public u8 status;
public override int ID { get { return (int)NetworkMessageID.G2U_RobotState; } } }
public partial class G2U_ImageChunk : NetworkMessage {
public u32 imageId;
public u32 frameTimeStamp;
public u16 nrows;
public u16 ncols;
public u16 chunkSize;
public u8 imageEncoding;
public u8 imageChunkCount;
public u8 chunkId;
public u8[] data = new u8[1024];
public override int ID { get { return (int)NetworkMessageID.G2U_ImageChunk; } } }




public partial class G2U_RobotObservedObject : NetworkMessage {
public u32 robotID;
public u32 objectID;
public u16 topLeft_x;
public u16 topLeft_y;
public u16 width;
public u16 height;
public override int ID { get { return (int)NetworkMessageID.G2U_RobotObservedObject; } } }

public partial class G2U_DeviceDetectedVisionMarker : NetworkMessage {
public u32 markerType;
public f32 x_upperLeft;
public f32 y_upperLeft;
public f32 x_lowerLeft;
public f32 y_lowerLeft;
public f32 x_upperRight;
public f32 y_upperRight;
public f32 x_lowerRight;
public f32 y_lowerRight;
public override int ID { get { return (int)NetworkMessageID.G2U_DeviceDetectedVisionMarker; } } }
public partial class G2U_PlaySound : NetworkMessage {
public char[] soundFilename = new char[128];
public u8 numLoops;
public u8 volume;
public override int ID { get { return (int)NetworkMessageID.G2U_PlaySound; } } }

public partial class G2U_StopSound : NetworkMessage {
public override int ID { get { return (int)NetworkMessageID.G2U_StopSound; } } }






public partial class U2G_ConnectToRobot { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.robotID) +
0; } } }


public partial class U2G_ConnectToUiDevice { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.deviceID) +
0; } } }


public partial class U2G_DisconnectFromUiDevice { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.deviceID) +
0; } } }



public partial class U2G_ForceAddRobot { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.ipAddress) +
ByteSerializer.GetSerializationLength(this.robotID) +
ByteSerializer.GetSerializationLength(this.isSimulated) +
0; } } }



public partial class U2G_StartEngine { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.asHost) +
ByteSerializer.GetSerializationLength(this.vizHostIP) +
0; } } }


public partial class U2G_DriveWheels { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.lwheel_speed_mmps) +
ByteSerializer.GetSerializationLength(this.rwheel_speed_mmps) +
0; } } }


public partial class U2G_TurnInPlace { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.angle_rad) +
ByteSerializer.GetSerializationLength(this.robotID) +
0; } } }


public partial class U2G_MoveHead { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.speed_rad_per_sec) +
0; } } }


public partial class U2G_MoveLift { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.speed_rad_per_sec) +
0; } } }


public partial class U2G_SetLiftHeight { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.height_mm) +
ByteSerializer.GetSerializationLength(this.max_speed_rad_per_sec) +
ByteSerializer.GetSerializationLength(this.accel_rad_per_sec2) +
0; } } }


public partial class U2G_SetHeadAngle { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.angle_rad) +
ByteSerializer.GetSerializationLength(this.max_speed_rad_per_sec) +
ByteSerializer.GetSerializationLength(this.accel_rad_per_sec2) +
0; } } }


public partial class U2G_StopAllMotors { public override int SerializationLength { get { return
0; } } }


public partial class U2G_ImageRequest { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.robotID) +
ByteSerializer.GetSerializationLength(this.mode) +
0; } } }





public partial class U2G_SetRobotImageSendMode { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.mode) +
ByteSerializer.GetSerializationLength(this.resolution) +
0; } } }


public partial class U2G_SaveImages { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.enableSave) +
0; } } }


public partial class U2G_EnableDisplay { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.enable) +
0; } } }


public partial class U2G_SetHeadlights { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.intensity) +
0; } } }


public partial class U2G_GotoPose { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.x_mm) +
ByteSerializer.GetSerializationLength(this.y_mm) +
ByteSerializer.GetSerializationLength(this.rad) +
ByteSerializer.GetSerializationLength(this.level) +
0; } } }


public partial class U2G_PlaceObjectOnGround { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.x_mm) +
ByteSerializer.GetSerializationLength(this.y_mm) +
ByteSerializer.GetSerializationLength(this.rad) +
ByteSerializer.GetSerializationLength(this.level) +
0; } } }


public partial class U2G_PlaceObjectOnGroundHere { public override int SerializationLength { get { return
0; } } }


public partial class U2G_ExecuteTestPlan { public override int SerializationLength { get { return
0; } } }


public partial class U2G_SelectNextObject { public override int SerializationLength { get { return
0; } } }


public partial class U2G_PickAndPlaceObject { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.objectID) +
ByteSerializer.GetSerializationLength(this.usePreDockPose) +
0; } } }


public partial class U2G_TraverseObject { public override int SerializationLength { get { return
0; } } }


public partial class U2G_ClearAllBlocks { public override int SerializationLength { get { return
0; } } }


public partial class U2G_ExecuteBehavior { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.behaviorMode) +
0; } } }


public partial class U2G_SetBehaviorState { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.behaviorState) +
0; } } }


public partial class U2G_AbortPath { public override int SerializationLength { get { return
0; } } }


public partial class U2G_AbortAll { public override int SerializationLength { get { return
0; } } }


public partial class U2G_DrawPoseMarker { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.x_mm) +
ByteSerializer.GetSerializationLength(this.y_mm) +
ByteSerializer.GetSerializationLength(this.rad) +
ByteSerializer.GetSerializationLength(this.level) +
0; } } }


public partial class U2G_ErasePoseMarker { public override int SerializationLength { get { return
0; } } }


public partial class U2G_SetHeadControllerGains { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.kp) +
ByteSerializer.GetSerializationLength(this.ki) +
ByteSerializer.GetSerializationLength(this.maxIntegralError) +
0; } } }


public partial class U2G_SetLiftControllerGains { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.kp) +
ByteSerializer.GetSerializationLength(this.ki) +
ByteSerializer.GetSerializationLength(this.maxIntegralError) +
0; } } }


public partial class U2G_SelectNextSoundScheme { public override int SerializationLength { get { return
0; } } }


public partial class U2G_StartTestMode { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.p1) +
ByteSerializer.GetSerializationLength(this.p2) +
ByteSerializer.GetSerializationLength(this.p3) +
ByteSerializer.GetSerializationLength(this.mode) +
0; } } }


public partial class U2G_IMURequest { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.length_ms) +
0; } } }


public partial class U2G_PlayAnimation { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.numLoops) +

ByteSerializer.GetSerializationLength(this.animationName) +
0; } } }


public partial class U2G_ReadAnimationFile { public override int SerializationLength { get { return
0; } } }


public partial class U2G_StartFaceTracking { public override int SerializationLength { get { return

ByteSerializer.GetSerializationLength(this.timeout_sec) +
0; } } }


public partial class U2G_StopFaceTracking { public override int SerializationLength { get { return
0; } } }


public partial class U2G_StartLookingForMarkers { public override int SerializationLength { get { return
0; } } }


public partial class U2G_StopLookingForMarkers { public override int SerializationLength { get { return
0; } } }


public partial class U2G_SetVisionSystemParams { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.autoexposureOn) +
ByteSerializer.GetSerializationLength(this.exposureTime) +
ByteSerializer.GetSerializationLength(this.integerCountsIncrement) +
ByteSerializer.GetSerializationLength(this.minExposureTime) +
ByteSerializer.GetSerializationLength(this.maxExposureTime) +
ByteSerializer.GetSerializationLength(this.percentileToMakeHigh) +
ByteSerializer.GetSerializationLength(this.limitFramerate) +
ByteSerializer.GetSerializationLength(this.highValue) +
0; } } }


public partial class U2G_SetFaceDetectParams { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.scaleFactor) +
ByteSerializer.GetSerializationLength(this.minNeighbors) +
ByteSerializer.GetSerializationLength(this.minObjectHeight) +
ByteSerializer.GetSerializationLength(this.minObjectWidth) +
ByteSerializer.GetSerializationLength(this.maxObjectHeight) +
ByteSerializer.GetSerializationLength(this.maxObjectWidth) +
0; } } }




public partial class G2U_RobotAvailable { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.robotID) +
0; } } }

public partial class G2U_UiDeviceAvailable { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.deviceID) +
0; } } }

public partial class G2U_RobotConnected { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.robotID) +
ByteSerializer.GetSerializationLength(this.successful) +
0; } } }

public partial class G2U_UiDeviceConnected { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.deviceID) +
ByteSerializer.GetSerializationLength(this.successful) +
0; } } }
public partial class G2U_RobotState { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.pose_x) +
ByteSerializer.GetSerializationLength(this.pose_y) +
ByteSerializer.GetSerializationLength(this.pose_z) +
ByteSerializer.GetSerializationLength(this.poseAngle_rad) +
ByteSerializer.GetSerializationLength(this.leftWheelSpeed_mmps) +
ByteSerializer.GetSerializationLength(this.rightWheelSpeed_mmps) +
ByteSerializer.GetSerializationLength(this.headAngle_rad) +
ByteSerializer.GetSerializationLength(this.liftHeight_mm) +
ByteSerializer.GetSerializationLength(this.status) +
0; } } }
public partial class G2U_ImageChunk { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.imageId) +
ByteSerializer.GetSerializationLength(this.frameTimeStamp) +
ByteSerializer.GetSerializationLength(this.nrows) +
ByteSerializer.GetSerializationLength(this.ncols) +
ByteSerializer.GetSerializationLength(this.chunkSize) +
ByteSerializer.GetSerializationLength(this.imageEncoding) +
ByteSerializer.GetSerializationLength(this.imageChunkCount) +
ByteSerializer.GetSerializationLength(this.chunkId) +
ByteSerializer.GetSerializationLength(this.data) +
0; } } }




public partial class G2U_RobotObservedObject { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.robotID) +
ByteSerializer.GetSerializationLength(this.objectID) +
ByteSerializer.GetSerializationLength(this.topLeft_x) +
ByteSerializer.GetSerializationLength(this.topLeft_y) +
ByteSerializer.GetSerializationLength(this.width) +
ByteSerializer.GetSerializationLength(this.height) +
0; } } }

public partial class G2U_DeviceDetectedVisionMarker { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.markerType) +
ByteSerializer.GetSerializationLength(this.x_upperLeft) +
ByteSerializer.GetSerializationLength(this.y_upperLeft) +
ByteSerializer.GetSerializationLength(this.x_lowerLeft) +
ByteSerializer.GetSerializationLength(this.y_lowerLeft) +
ByteSerializer.GetSerializationLength(this.x_upperRight) +
ByteSerializer.GetSerializationLength(this.y_upperRight) +
ByteSerializer.GetSerializationLength(this.x_lowerRight) +
ByteSerializer.GetSerializationLength(this.y_lowerRight) +
0; } } }
public partial class G2U_PlaySound { public override int SerializationLength { get { return
ByteSerializer.GetSerializationLength(this.soundFilename) +
ByteSerializer.GetSerializationLength(this.numLoops) +
ByteSerializer.GetSerializationLength(this.volume) +
0; } } }

public partial class G2U_StopSound { public override int SerializationLength { get { return
0; } } }






public partial class U2G_ConnectToRobot { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.robotID);
} }


public partial class U2G_ConnectToUiDevice { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.deviceID);
} }


public partial class U2G_DisconnectFromUiDevice { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.deviceID);
} }



public partial class U2G_ForceAddRobot { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.ipAddress);
serializer.Serialize(this.robotID);
serializer.Serialize(this.isSimulated);
} }



public partial class U2G_StartEngine { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.asHost);
serializer.Serialize(this.vizHostIP);
} }


public partial class U2G_DriveWheels { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.lwheel_speed_mmps);
serializer.Serialize(this.rwheel_speed_mmps);
} }


public partial class U2G_TurnInPlace { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.angle_rad);
serializer.Serialize(this.robotID);
} }


public partial class U2G_MoveHead { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.speed_rad_per_sec);
} }


public partial class U2G_MoveLift { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.speed_rad_per_sec);
} }


public partial class U2G_SetLiftHeight { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.height_mm);
serializer.Serialize(this.max_speed_rad_per_sec);
serializer.Serialize(this.accel_rad_per_sec2);
} }


public partial class U2G_SetHeadAngle { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.angle_rad);
serializer.Serialize(this.max_speed_rad_per_sec);
serializer.Serialize(this.accel_rad_per_sec2);
} }


public partial class U2G_StopAllMotors { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_ImageRequest { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.robotID);
serializer.Serialize(this.mode);
} }





public partial class U2G_SetRobotImageSendMode { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.mode);
serializer.Serialize(this.resolution);
} }


public partial class U2G_SaveImages { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.enableSave);
} }


public partial class U2G_EnableDisplay { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.enable);
} }


public partial class U2G_SetHeadlights { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.intensity);
} }


public partial class U2G_GotoPose { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.x_mm);
serializer.Serialize(this.y_mm);
serializer.Serialize(this.rad);
serializer.Serialize(this.level);
} }


public partial class U2G_PlaceObjectOnGround { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.x_mm);
serializer.Serialize(this.y_mm);
serializer.Serialize(this.rad);
serializer.Serialize(this.level);
} }


public partial class U2G_PlaceObjectOnGroundHere { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_ExecuteTestPlan { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_SelectNextObject { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_PickAndPlaceObject { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.objectID);
serializer.Serialize(this.usePreDockPose);
} }


public partial class U2G_TraverseObject { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_ClearAllBlocks { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_ExecuteBehavior { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.behaviorMode);
} }


public partial class U2G_SetBehaviorState { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.behaviorState);
} }


public partial class U2G_AbortPath { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_AbortAll { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_DrawPoseMarker { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.x_mm);
serializer.Serialize(this.y_mm);
serializer.Serialize(this.rad);
serializer.Serialize(this.level);
} }


public partial class U2G_ErasePoseMarker { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_SetHeadControllerGains { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.kp);
serializer.Serialize(this.ki);
serializer.Serialize(this.maxIntegralError);
} }


public partial class U2G_SetLiftControllerGains { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.kp);
serializer.Serialize(this.ki);
serializer.Serialize(this.maxIntegralError);
} }


public partial class U2G_SelectNextSoundScheme { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_StartTestMode { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.p1);
serializer.Serialize(this.p2);
serializer.Serialize(this.p3);
serializer.Serialize(this.mode);
} }


public partial class U2G_IMURequest { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.length_ms);
} }


public partial class U2G_PlayAnimation { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.numLoops);

serializer.Serialize(this.animationName);
} }


public partial class U2G_ReadAnimationFile { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_StartFaceTracking { public override void Serialize(ByteSerializer serializer) {

serializer.Serialize(this.timeout_sec);
} }


public partial class U2G_StopFaceTracking { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_StartLookingForMarkers { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_StopLookingForMarkers { public override void Serialize(ByteSerializer serializer) {
} }


public partial class U2G_SetVisionSystemParams { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.autoexposureOn);
serializer.Serialize(this.exposureTime);
serializer.Serialize(this.integerCountsIncrement);
serializer.Serialize(this.minExposureTime);
serializer.Serialize(this.maxExposureTime);
serializer.Serialize(this.percentileToMakeHigh);
serializer.Serialize(this.limitFramerate);
serializer.Serialize(this.highValue);
} }


public partial class U2G_SetFaceDetectParams { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.scaleFactor);
serializer.Serialize(this.minNeighbors);
serializer.Serialize(this.minObjectHeight);
serializer.Serialize(this.minObjectWidth);
serializer.Serialize(this.maxObjectHeight);
serializer.Serialize(this.maxObjectWidth);
} }




public partial class G2U_RobotAvailable { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.robotID);
} }

public partial class G2U_UiDeviceAvailable { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.deviceID);
} }

public partial class G2U_RobotConnected { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.robotID);
serializer.Serialize(this.successful);
} }

public partial class G2U_UiDeviceConnected { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.deviceID);
serializer.Serialize(this.successful);
} }
public partial class G2U_RobotState { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.pose_x);
serializer.Serialize(this.pose_y);
serializer.Serialize(this.pose_z);
serializer.Serialize(this.poseAngle_rad);
serializer.Serialize(this.leftWheelSpeed_mmps);
serializer.Serialize(this.rightWheelSpeed_mmps);
serializer.Serialize(this.headAngle_rad);
serializer.Serialize(this.liftHeight_mm);
serializer.Serialize(this.status);
} }
public partial class G2U_ImageChunk { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.imageId);
serializer.Serialize(this.frameTimeStamp);
serializer.Serialize(this.nrows);
serializer.Serialize(this.ncols);
serializer.Serialize(this.chunkSize);
serializer.Serialize(this.imageEncoding);
serializer.Serialize(this.imageChunkCount);
serializer.Serialize(this.chunkId);
serializer.Serialize(this.data);
} }




public partial class G2U_RobotObservedObject { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.robotID);
serializer.Serialize(this.objectID);
serializer.Serialize(this.topLeft_x);
serializer.Serialize(this.topLeft_y);
serializer.Serialize(this.width);
serializer.Serialize(this.height);
} }

public partial class G2U_DeviceDetectedVisionMarker { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.markerType);
serializer.Serialize(this.x_upperLeft);
serializer.Serialize(this.y_upperLeft);
serializer.Serialize(this.x_lowerLeft);
serializer.Serialize(this.y_lowerLeft);
serializer.Serialize(this.x_upperRight);
serializer.Serialize(this.y_upperRight);
serializer.Serialize(this.x_lowerRight);
serializer.Serialize(this.y_lowerRight);
} }
public partial class G2U_PlaySound { public override void Serialize(ByteSerializer serializer) {
serializer.Serialize(this.soundFilename);
serializer.Serialize(this.numLoops);
serializer.Serialize(this.volume);
} }

public partial class G2U_StopSound { public override void Serialize(ByteSerializer serializer) {
} }






public partial class U2G_ConnectToRobot { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.robotID);
} }


public partial class U2G_ConnectToUiDevice { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.deviceID);
} }


public partial class U2G_DisconnectFromUiDevice { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.deviceID);
} }



public partial class U2G_ForceAddRobot { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(this.ipAddress);
serializer.Deserialize(out this.robotID);
serializer.Deserialize(out this.isSimulated);
} }



public partial class U2G_StartEngine { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.asHost);
serializer.Deserialize(this.vizHostIP);
} }


public partial class U2G_DriveWheels { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.lwheel_speed_mmps);
serializer.Deserialize(out this.rwheel_speed_mmps);
} }


public partial class U2G_TurnInPlace { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.angle_rad);
serializer.Deserialize(out this.robotID);
} }


public partial class U2G_MoveHead { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.speed_rad_per_sec);
} }


public partial class U2G_MoveLift { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.speed_rad_per_sec);
} }


public partial class U2G_SetLiftHeight { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.height_mm);
serializer.Deserialize(out this.max_speed_rad_per_sec);
serializer.Deserialize(out this.accel_rad_per_sec2);
} }


public partial class U2G_SetHeadAngle { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.angle_rad);
serializer.Deserialize(out this.max_speed_rad_per_sec);
serializer.Deserialize(out this.accel_rad_per_sec2);
} }


public partial class U2G_StopAllMotors { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_ImageRequest { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.robotID);
serializer.Deserialize(out this.mode);
} }





public partial class U2G_SetRobotImageSendMode { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.mode);
serializer.Deserialize(out this.resolution);
} }


public partial class U2G_SaveImages { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.enableSave);
} }


public partial class U2G_EnableDisplay { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.enable);
} }


public partial class U2G_SetHeadlights { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.intensity);
} }


public partial class U2G_GotoPose { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.x_mm);
serializer.Deserialize(out this.y_mm);
serializer.Deserialize(out this.rad);
serializer.Deserialize(out this.level);
} }


public partial class U2G_PlaceObjectOnGround { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.x_mm);
serializer.Deserialize(out this.y_mm);
serializer.Deserialize(out this.rad);
serializer.Deserialize(out this.level);
} }


public partial class U2G_PlaceObjectOnGroundHere { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_ExecuteTestPlan { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_SelectNextObject { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_PickAndPlaceObject { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.objectID);
serializer.Deserialize(out this.usePreDockPose);
} }


public partial class U2G_TraverseObject { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_ClearAllBlocks { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_ExecuteBehavior { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.behaviorMode);
} }


public partial class U2G_SetBehaviorState { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.behaviorState);
} }


public partial class U2G_AbortPath { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_AbortAll { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_DrawPoseMarker { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.x_mm);
serializer.Deserialize(out this.y_mm);
serializer.Deserialize(out this.rad);
serializer.Deserialize(out this.level);
} }


public partial class U2G_ErasePoseMarker { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_SetHeadControllerGains { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.kp);
serializer.Deserialize(out this.ki);
serializer.Deserialize(out this.maxIntegralError);
} }


public partial class U2G_SetLiftControllerGains { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.kp);
serializer.Deserialize(out this.ki);
serializer.Deserialize(out this.maxIntegralError);
} }


public partial class U2G_SelectNextSoundScheme { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_StartTestMode { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.p1);
serializer.Deserialize(out this.p2);
serializer.Deserialize(out this.p3);
serializer.Deserialize(out this.mode);
} }


public partial class U2G_IMURequest { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.length_ms);
} }


public partial class U2G_PlayAnimation { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.numLoops);

serializer.Deserialize(this.animationName);
} }


public partial class U2G_ReadAnimationFile { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_StartFaceTracking { public override void Deserialize(ByteSerializer serializer) {

serializer.Deserialize(out this.timeout_sec);
} }


public partial class U2G_StopFaceTracking { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_StartLookingForMarkers { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_StopLookingForMarkers { public override void Deserialize(ByteSerializer serializer) {
} }


public partial class U2G_SetVisionSystemParams { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.autoexposureOn);
serializer.Deserialize(out this.exposureTime);
serializer.Deserialize(out this.integerCountsIncrement);
serializer.Deserialize(out this.minExposureTime);
serializer.Deserialize(out this.maxExposureTime);
serializer.Deserialize(out this.percentileToMakeHigh);
serializer.Deserialize(out this.limitFramerate);
serializer.Deserialize(out this.highValue);
} }


public partial class U2G_SetFaceDetectParams { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.scaleFactor);
serializer.Deserialize(out this.minNeighbors);
serializer.Deserialize(out this.minObjectHeight);
serializer.Deserialize(out this.minObjectWidth);
serializer.Deserialize(out this.maxObjectHeight);
serializer.Deserialize(out this.maxObjectWidth);
} }




public partial class G2U_RobotAvailable { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.robotID);
} }

public partial class G2U_UiDeviceAvailable { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.deviceID);
} }

public partial class G2U_RobotConnected { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.robotID);
serializer.Deserialize(out this.successful);
} }

public partial class G2U_UiDeviceConnected { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.deviceID);
serializer.Deserialize(out this.successful);
} }
public partial class G2U_RobotState { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.pose_x);
serializer.Deserialize(out this.pose_y);
serializer.Deserialize(out this.pose_z);
serializer.Deserialize(out this.poseAngle_rad);
serializer.Deserialize(out this.leftWheelSpeed_mmps);
serializer.Deserialize(out this.rightWheelSpeed_mmps);
serializer.Deserialize(out this.headAngle_rad);
serializer.Deserialize(out this.liftHeight_mm);
serializer.Deserialize(out this.status);
} }
public partial class G2U_ImageChunk { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.imageId);
serializer.Deserialize(out this.frameTimeStamp);
serializer.Deserialize(out this.nrows);
serializer.Deserialize(out this.ncols);
serializer.Deserialize(out this.chunkSize);
serializer.Deserialize(out this.imageEncoding);
serializer.Deserialize(out this.imageChunkCount);
serializer.Deserialize(out this.chunkId);
serializer.Deserialize(this.data);
} }




public partial class G2U_RobotObservedObject { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.robotID);
serializer.Deserialize(out this.objectID);
serializer.Deserialize(out this.topLeft_x);
serializer.Deserialize(out this.topLeft_y);
serializer.Deserialize(out this.width);
serializer.Deserialize(out this.height);
} }

public partial class G2U_DeviceDetectedVisionMarker { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(out this.markerType);
serializer.Deserialize(out this.x_upperLeft);
serializer.Deserialize(out this.y_upperLeft);
serializer.Deserialize(out this.x_lowerLeft);
serializer.Deserialize(out this.y_lowerLeft);
serializer.Deserialize(out this.x_upperRight);
serializer.Deserialize(out this.y_upperRight);
serializer.Deserialize(out this.x_lowerRight);
serializer.Deserialize(out this.y_lowerRight);
} }
public partial class G2U_PlaySound { public override void Deserialize(ByteSerializer serializer) {
serializer.Deserialize(this.soundFilename);
serializer.Deserialize(out this.numLoops);
serializer.Deserialize(out this.volume);
} }

public partial class G2U_StopSound { public override void Deserialize(ByteSerializer serializer) {
} }

public static class NetworkMessageCreation {

  public static NetworkMessage Allocate(int messageTypeID) {
    switch ((NetworkMessageID)messageTypeID) {
      default:
        return null;






case NetworkMessageID.U2G_ConnectToRobot: return new U2G_ConnectToRobot();




case NetworkMessageID.U2G_ConnectToUiDevice: return new U2G_ConnectToUiDevice();




case NetworkMessageID.U2G_DisconnectFromUiDevice: return new U2G_DisconnectFromUiDevice();





case NetworkMessageID.U2G_ForceAddRobot: return new U2G_ForceAddRobot();







case NetworkMessageID.U2G_StartEngine: return new U2G_StartEngine();





case NetworkMessageID.U2G_DriveWheels: return new U2G_DriveWheels();





case NetworkMessageID.U2G_TurnInPlace: return new U2G_TurnInPlace();





case NetworkMessageID.U2G_MoveHead: return new U2G_MoveHead();




case NetworkMessageID.U2G_MoveLift: return new U2G_MoveLift();




case NetworkMessageID.U2G_SetLiftHeight: return new U2G_SetLiftHeight();






case NetworkMessageID.U2G_SetHeadAngle: return new U2G_SetHeadAngle();






case NetworkMessageID.U2G_StopAllMotors: return new U2G_StopAllMotors();



case NetworkMessageID.U2G_ImageRequest: return new U2G_ImageRequest();
case NetworkMessageID.U2G_SetRobotImageSendMode: return new U2G_SetRobotImageSendMode();





case NetworkMessageID.U2G_SaveImages: return new U2G_SaveImages();




case NetworkMessageID.U2G_EnableDisplay: return new U2G_EnableDisplay();




case NetworkMessageID.U2G_SetHeadlights: return new U2G_SetHeadlights();




case NetworkMessageID.U2G_GotoPose: return new U2G_GotoPose();







case NetworkMessageID.U2G_PlaceObjectOnGround: return new U2G_PlaceObjectOnGround();







case NetworkMessageID.U2G_PlaceObjectOnGroundHere: return new U2G_PlaceObjectOnGroundHere();



case NetworkMessageID.U2G_ExecuteTestPlan: return new U2G_ExecuteTestPlan();



case NetworkMessageID.U2G_SelectNextObject: return new U2G_SelectNextObject();



case NetworkMessageID.U2G_PickAndPlaceObject: return new U2G_PickAndPlaceObject();





case NetworkMessageID.U2G_TraverseObject: return new U2G_TraverseObject();



case NetworkMessageID.U2G_ClearAllBlocks: return new U2G_ClearAllBlocks();



case NetworkMessageID.U2G_ExecuteBehavior: return new U2G_ExecuteBehavior();




case NetworkMessageID.U2G_SetBehaviorState: return new U2G_SetBehaviorState();




case NetworkMessageID.U2G_AbortPath: return new U2G_AbortPath();



case NetworkMessageID.U2G_AbortAll: return new U2G_AbortAll();



case NetworkMessageID.U2G_DrawPoseMarker: return new U2G_DrawPoseMarker();







case NetworkMessageID.U2G_ErasePoseMarker: return new U2G_ErasePoseMarker();



case NetworkMessageID.U2G_SetHeadControllerGains: return new U2G_SetHeadControllerGains();






case NetworkMessageID.U2G_SetLiftControllerGains: return new U2G_SetLiftControllerGains();






case NetworkMessageID.U2G_SelectNextSoundScheme: return new U2G_SelectNextSoundScheme();



case NetworkMessageID.U2G_StartTestMode: return new U2G_StartTestMode();







case NetworkMessageID.U2G_IMURequest: return new U2G_IMURequest();




case NetworkMessageID.U2G_PlayAnimation: return new U2G_PlayAnimation();






case NetworkMessageID.U2G_ReadAnimationFile: return new U2G_ReadAnimationFile();



case NetworkMessageID.U2G_StartFaceTracking: return new U2G_StartFaceTracking();





case NetworkMessageID.U2G_StopFaceTracking: return new U2G_StopFaceTracking();



case NetworkMessageID.U2G_StartLookingForMarkers: return new U2G_StartLookingForMarkers();



case NetworkMessageID.U2G_StopLookingForMarkers: return new U2G_StopLookingForMarkers();



case NetworkMessageID.U2G_SetVisionSystemParams: return new U2G_SetVisionSystemParams();
case NetworkMessageID.U2G_SetFaceDetectParams: return new U2G_SetFaceDetectParams();




case NetworkMessageID.G2U_RobotAvailable: return new G2U_RobotAvailable();



case NetworkMessageID.G2U_UiDeviceAvailable: return new G2U_UiDeviceAvailable();



case NetworkMessageID.G2U_RobotConnected: return new G2U_RobotConnected();




case NetworkMessageID.G2U_UiDeviceConnected: return new G2U_UiDeviceConnected();
case NetworkMessageID.G2U_RobotState: return new G2U_RobotState();
case NetworkMessageID.G2U_ImageChunk: return new G2U_ImageChunk();
case NetworkMessageID.G2U_RobotObservedObject: return new G2U_RobotObservedObject();
case NetworkMessageID.G2U_DeviceDetectedVisionMarker: return new G2U_DeviceDetectedVisionMarker();
case NetworkMessageID.G2U_PlaySound: return new G2U_PlaySound();





case NetworkMessageID.G2U_StopSound: return new G2U_StopSound();
 }
  }
}
