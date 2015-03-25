using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class Robot
{
	public byte ID { get; private set; }
	public float headAngle_rad { get; private set; }
	public float poseAngle_rad { get; private set; }
	public float leftWheelSpeed_mmps { get; private set; }
	public float rightWheelSpeed_mmps { get; private set; }
	public float liftHeight_mm { get; private set; }

	public Vector3 WorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }
	public Vector3 Forward { 
		get {
			return Rotation * Vector3.right;		
		}
	}
	public Vector3 Right { 
		get {
			return Rotation * -Vector3.up;		
		}
	}

	public StatusFlag status { get; private set; }
	public float batteryPercent { get; private set; }
	public int carryingObjectID { get; private set; }
	public List<ObservedObject> observedObjects { get; private set; }
	public List<ObservedObject> knownObjects { get; private set; }
	public List<ObservedObject> selectedObjects { get; private set; }
	public List<ObservedObject> lastSelectedObjects { get; private set; }
	public List<ObservedObject> lastObservedObjects { get; private set; }
	public ObservedObject lastObjectHeadTracked;

	// er, should be 5?
	private const float MaxVoltage = 5.0f;
	private const float defaultHeadAngle = 0f;

	[System.FlagsAttribute]
	public enum StatusFlag
	{
		NONE                    = 0,
		IS_MOVING               = 0x1,  // Head, lift, or wheels
		IS_CARRYING_BLOCK       = 0x2,
		IS_PICKING_OR_PLACING   = 0x4,
		IS_PICKED_UP            = 0x8,
		IS_PROX_FORWARD_BLOCKED = 0x10,
		IS_PROX_SIDE_BLOCKED    = 0x20,
		IS_ANIMATING            = 0x40
	};

	public bool isBusy
	{
		get
		{
			return Status( StatusFlag.IS_PICKING_OR_PLACING ) || Status( StatusFlag.IS_PICKED_UP ) || Status( StatusFlag.IS_ANIMATING );
		}
	}

	public bool Status( StatusFlag s )
	{
		return (status & s) == s;
	}

	public Robot( byte robotID )
	{
		ID = robotID;
		selectedObjects = new List<ObservedObject>();
		lastSelectedObjects = new List<ObservedObject>();
		lastObjectHeadTracked = null;
		observedObjects = new List<ObservedObject>();
		lastObservedObjects = new List<ObservedObject>();
		knownObjects = new List<ObservedObject>();

		RobotEngineManager.instance.DisconnectedFromClient += Reset;
	}

	private void Reset( DisconnectionReason reason = DisconnectionReason.None )
	{
		ClearData();
	}
	
	public void ClearData()
	{
		selectedObjects.Clear();
		lastSelectedObjects.Clear();
		lastObjectHeadTracked = null;
		observedObjects.Clear();
		lastObservedObjects.Clear();
		knownObjects.Clear();
		status = StatusFlag.NONE;
		WorldPosition = Vector3.zero;
		Rotation = Quaternion.identity;
		carryingObjectID = -1;
	}

	public void UpdateInfo( G2U_RobotState message )
	{
		headAngle_rad = message.headAngle_rad;
		poseAngle_rad = message.poseAngle_rad;
		leftWheelSpeed_mmps = message.leftWheelSpeed_mmps;
		rightWheelSpeed_mmps = message.rightWheelSpeed_mmps;
		liftHeight_mm = message.liftHeight_mm;
		WorldPosition = new Vector3(message.pose_x, message.pose_y,	message.pose_z);
		status = (StatusFlag)message.status;
		batteryPercent = (message.batteryVoltage / MaxVoltage);
		carryingObjectID = message.carryingObjectID;

		Rotation = new Quaternion(message.pose_quaternion1, message.pose_quaternion2, message.pose_quaternion3, message.pose_quaternion0);
	}

	public void UpdateObservedObjectInfo( G2U_RobotObservedObject message )
	{

		if(message.objectFamily == 0) {
			//Debug.LogWarning("UpdateObservedObjectInfo received message about the Mat!" );
			return;
		}

		ObservedObject knownObject = knownObjects.Find( x => x.ID == message.objectID );

		//Debug.Log( "found " + message.objectID );

		if( knownObject == null )
		{
			knownObject = new ObservedObject();

			knownObjects.Add( knownObject );
		}

		knownObject.UpdateInfo( message );

		ObservedObject observedObject = observedObjects.Find( x => x.ID == message.objectID );

		if(observedObject == null) {
			observedObjects.Add(knownObject);
		}
	}

	public void DriveWheels(float leftWheelSpeedMmps, float rightWheelSpeedMmps)
	{
		//Debug.Log("DriveWheels(leftWheelSpeedMmps:"+leftWheelSpeedMmps+", rightWheelSpeedMmps:"+rightWheelSpeedMmps+")");
		U2G_DriveWheels message = new U2G_DriveWheels ();
		message.lwheel_speed_mmps = leftWheelSpeedMmps;
		message.rwheel_speed_mmps = rightWheelSpeedMmps;
		
		RobotEngineManager.instance.channel.Send (new U2G_Message{DriveWheels=message});
	}

	public void PlaceObjectOnGroundHere()
	{
		Debug.Log( "Place Object On Ground Here" );
		
		U2G_PlaceObjectOnGroundHere message = new U2G_PlaceObjectOnGroundHere ();
		
		RobotEngineManager.instance.channel.Send (new U2G_Message{PlaceObjectOnGroundHere=message});
	}

	public void SetHeadAngle( float angle_rad = defaultHeadAngle )
	{
		//Debug.Log( "Set Head Angle " + angle_rad );
		
		U2G_SetHeadAngle message = new U2G_SetHeadAngle();
		message.angle_rad = angle_rad;
		message.accel_rad_per_sec2 = 2f;
		message.max_speed_rad_per_sec = 5f;
		
		RobotEngineManager.instance.channel.Send( new U2G_Message { SetHeadAngle = message } );
		
		lastObjectHeadTracked = null;
	}
	
	public void TrackHeadToObject( ObservedObject observedObject )
	{
		if( lastObjectHeadTracked == null || lastObjectHeadTracked.ID != observedObject.ID )
		{
			Debug.Log( "Track Head To Object " + observedObject.ID );
			
			U2G_TrackHeadToObject message = new U2G_TrackHeadToObject();
			message.objectID = (uint)observedObject.ID;
			message.robotID = ID;
			
			RobotEngineManager.instance.channel.Send( new U2G_Message { TrackHeadToObject = message } );
			
			lastObjectHeadTracked = observedObject;
		}
	}
	
	public void PickAndPlaceObject( int index = 0, bool usePreDockPose = false, bool useManualSpeed = false )
	{
		Debug.Log( "Pick And Place Object index(" + index + ") usePreDockPose " + usePreDockPose + " useManualSpeed " + useManualSpeed );
		
		U2G_PickAndPlaceObject message = new U2G_PickAndPlaceObject();
		message.objectID = selectedObjects[index].ID;
		message.usePreDockPose = System.Convert.ToByte( usePreDockPose );
		message.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ PickAndPlaceObject = message } );
		
		//current.observedObjects.Clear();
		lastObjectHeadTracked = null;
	}
	
	public void SetLiftHeight( float height )
	{
		Debug.Log( "Set Lift Height " + height );
		
		U2G_SetLiftHeight message = new U2G_SetLiftHeight();
		message.accel_rad_per_sec2 = 5f;
		message.max_speed_rad_per_sec = 10f;
		message.height_mm = height;
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ SetLiftHeight = message } );
	}
	
	public void SetRobotCarryingObject( int objectID = -1 )
	{
		Debug.Log( "Set Robot Carrying Object" );
		
		U2G_SetRobotCarryingObject message = new U2G_SetRobotCarryingObject();
		
		message.robotID = ID;
		message.objectID = objectID;
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ SetRobotCarryingObject = message } );
		lastObjectHeadTracked = null;
		selectedObjects.Clear();
		
		SetLiftHeight( 0f );
		SetHeadAngle( defaultHeadAngle );
	}
	
	public void ClearAllBlocks()
	{
		Debug.Log( "Clear All Blocks" );
		
		U2G_ClearAllBlocks message = new U2G_ClearAllBlocks();
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ ClearAllBlocks = message } );
		Reset();
		
		SetLiftHeight( 0f );
		SetHeadAngle( defaultHeadAngle );
	}
	
	public void VisionWhileMoving( bool enable )
	{
		Debug.Log( "Vision While Moving " + enable );
		
		U2G_VisionWhileMoving message = new U2G_VisionWhileMoving();
		message.enable = System.Convert.ToByte( enable );
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ VisionWhileMoving = message } );
	}
	
	public void RequestImage( RobotEngineManager.CameraResolution resolution, RobotEngineManager.ImageSendMode_t mode )
	{
		U2G_SetRobotImageSendMode message = new U2G_SetRobotImageSendMode ();
		message.resolution = (byte)resolution;
		message.mode = (byte)mode;
		
		RobotEngineManager.instance.channel.Send (new U2G_Message{SetRobotImageSendMode = message});
		
		U2G_ImageRequest message2 = new U2G_ImageRequest();
		message2.robotID = ID;
		message2.mode = (byte)mode;
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ ImageRequest = message2 } );
		
		Debug.Log( "image request message sent with " + mode + " at " + resolution );
	}
	
	public void StopAllMotors()
	{
		U2G_StopAllMotors message = new U2G_StopAllMotors ();
		
		RobotEngineManager.instance.channel.Send(new U2G_Message{ StopAllMotors = message } );
	}
	
	public void TurnInPlace(float angle_rad)
	{
		U2G_TurnInPlace message = new U2G_TurnInPlace ();
		message.robotID = ID;
		message.angle_rad = angle_rad;
		
		Debug.Log("TurnInPlace(robotID:"+ID+", angle_rad:"+angle_rad+")");
		RobotEngineManager.instance.channel.Send( new U2G_Message{ TurnInPlace = message } );
	}

	public void TraverseObject( int objectID, bool usePreDockPose = false, bool useManualSpeed = false )
	{
		Debug.Log( "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose );
		
		U2G_TraverseObject message = new U2G_TraverseObject();
		message.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		message.usePreDockPose = System.Convert.ToByte( usePreDockPose );
		
		RobotEngineManager.instance.channel.Send( new U2G_Message{ TraverseObject = message } );
	}
}
