using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public class Robot
{
	public byte ID { get; private set; }
	public float headAngle_rad { get; private set; }
	public float poseAngle_rad { get; private set; }
	public float leftWheelSpeed_mmps { get; private set; }
	public float rightWheelSpeed_mmps { get; private set; }
	public float liftHeight_mm { get; private set; }

	public Vector3 WorldPosition { get; private set; }
	public Vector3 LastWorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }
	public Quaternion LastRotation { get; private set; }
	public Vector3 Forward { get { return Rotation * Vector3.right;	} }
	public Vector3 Right { get { return Rotation * -Vector3.up;	} }

	public StatusFlag status { get; private set; }
	public float batteryPercent { get; private set; }
	public List<ObservedObject> markersVisibleObjects { get; private set; }
	public List<ObservedObject> observedObjects { get; private set; }
	public List<ObservedObject> knownObjects { get; private set; }
	public List<ObservedObject> selectedObjects { get; private set; }
	public List<ObservedObject> lastSelectedObjects { get; private set; }
	public List<ObservedObject> lastObservedObjects { get; private set; }
	public List<ObservedObject> lastMarkersVisibleObjects { get; private set; }


	private ObservedObject _targetLockedObject = null;
	public ObservedObject targetLockedObject {
		get { return _targetLockedObject; }
		set {

			//Debug.Log("Robot targetLockedObject = " + (value != null ? value.RobotID.ToString() : "null") );

			_targetLockedObject = value;
		}
	}

	public RobotActionType lastActionRequested;
	public bool searching;

	private int carryingObjectID;
	private int headTrackingObjectID;
	private int lastHeadTrackingObjectID;

	private float headAngleRequested;
	private float lastHeadAngleRequestTime;
	private float liftHeightRequested;
	private float lastLiftHeightRequestTime;


	private ObservedObject _carryingObject;
	public ObservedObject carryingObject
	{
		get
		{
			if( _carryingObject != carryingObjectID ) _carryingObject = knownObjects.Find( x => x == carryingObjectID );

			return _carryingObject;
		}
	}

	private ObservedObject _headTrackingObject;
	public ObservedObject headTrackingObject
	{
		get
		{
			if( _headTrackingObject != headTrackingObjectID ) _headTrackingObject = knownObjects.Find( x => x == headTrackingObjectID );
			
			return _headTrackingObject;
		}
	}

	public enum ObservedObjectListType {
		OBSERVED_RECENTLY,
		MARKERS_SEEN,
		KNOWN,
		KNOWN_IN_RANGE
	}
	
	protected ObservedObjectListType observedObjectListType = ObservedObjectListType.MARKERS_SEEN;
	protected float objectPertinenceRange = 220f;
	
	protected ActionPanel actionPanel;
	
	private List<ObservedObject> _pertinentObjects = new List<ObservedObject>(); 
	private int pertinenceStamp = -1;

	private readonly Predicate<ObservedObject> IsNotInRange_callback;
	private readonly Predicate<ObservedObject> IsNotInZRange_callback;
	private readonly Comparison<ObservedObject> SortByDistance_callback;
	private const float Z_RANGE_LIMIT = 2f * CozmoUtil.BLOCK_LENGTH_MM;
	
	// Note: Instantiating delegates and List<T>.FindAll both allocate memory.
	// May want to change before releasing in production.
	// It's fine for prototyping.
	public List<ObservedObject> pertinentObjects
	{
		get
		{
			if(pertinenceStamp == Time.frameCount) return _pertinentObjects;
			pertinenceStamp = Time.frameCount;
			
			_pertinentObjects.Clear();
			
			switch(observedObjectListType) {
				case ObservedObjectListType.MARKERS_SEEN: 
					_pertinentObjects.AddRange(markersVisibleObjects);
					break;
				case ObservedObjectListType.OBSERVED_RECENTLY: 
					_pertinentObjects.AddRange(observedObjects);
					break;
				case ObservedObjectListType.KNOWN: 
					_pertinentObjects.AddRange(knownObjects);
					break;
				case ObservedObjectListType.KNOWN_IN_RANGE: 
					_pertinentObjects.AddRange (knownObjects);
					_pertinentObjects.RemoveAll (IsNotInRange_callback);
					break;
			}

			_pertinentObjects.RemoveAll (IsNotInZRange_callback);
			_pertinentObjects.Sort (SortByDistance_callback);
			return _pertinentObjects;
		}
	}

	// er, should be 5?
	private const float MaxVoltage = 5.0f;

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
		IS_ANIMATING            = 0x40,
		IS_PERFORMING_ACTION    = 0x80
	};

	[System.NonSerialized] public float localBusyTimer = 0f;
	public bool isBusy
	{
		get
		{
			return localBusyTimer > 0f 
				|| Status( StatusFlag.IS_PERFORMING_ACTION )
				|| Status( StatusFlag.IS_ANIMATING ) 
				|| Status( StatusFlag.IS_PICKED_UP );
		}

		set
		{
			localBusyTimer = value ? float.MaxValue : 0;
			if( value )
			{
				DriveWheels(0,0); 
			}
		}
	}

	public bool Status( StatusFlag s )
	{
		return (status & s) == s;
	}

	private Robot()
	{
		IsNotInRange_callback = IsNotInRange;
		IsNotInZRange_callback = IsNotInZRange;
		SortByDistance_callback = SortByDistance;
	}

	public Robot( byte robotID )
		: this()
	{
		ID = robotID;
		const int initialSize = 64;
		selectedObjects = new List<ObservedObject>(initialSize);
		lastSelectedObjects = new List<ObservedObject>(initialSize);
		observedObjects = new List<ObservedObject>(initialSize);
		lastObservedObjects = new List<ObservedObject>(initialSize);
		markersVisibleObjects = new List<ObservedObject>(initialSize);
		lastMarkersVisibleObjects = new List<ObservedObject>(initialSize);
		knownObjects = new List<ObservedObject>(initialSize);

		ClearData();

		RobotEngineManager.instance.DisconnectedFromClient += Reset;

		RefreshObjectPertinence();
	}

	public void CooldownTimers(float delta) {
		if(localBusyTimer > 0f) {
			localBusyTimer -= delta;
		}
	}

	public void RefreshObjectPertinence() {

		int objectPertinenceOverride = OptionsScreen.GetObjectPertinenceTypeOverride();

		if( objectPertinenceOverride >= 0 )
		{
			observedObjectListType = (ObservedObjectListType)objectPertinenceOverride;
			Debug.Log("CozmoVision.OnEnable observedObjectListType("+observedObjectListType+")");
		}

		float objectPertinenceRangeOverride = OptionsScreen.GetObjectPertinenceRangeOverride();
		if( objectPertinenceRangeOverride >= 0 )
		{
			objectPertinenceRange = objectPertinenceRangeOverride;
			Debug.Log("CozmoVision.OnEnable objectPertinenceRange("+objectPertinenceRangeOverride+")");
		}
	}

	private void Reset( DisconnectionReason reason = DisconnectionReason.None )
	{
		ClearData();
	}
	
	public void ClearData()
	{
		selectedObjects.Clear();
		lastSelectedObjects.Clear();
		observedObjects.Clear();
		lastObservedObjects.Clear();
		markersVisibleObjects.Clear();
		lastMarkersVisibleObjects.Clear();
		knownObjects.Clear();
		status = StatusFlag.NONE;
		WorldPosition = Vector3.zero;
		Rotation = Quaternion.identity;
		carryingObjectID = -1;
		headTrackingObjectID = -1;
		lastHeadTrackingObjectID = -1;
		targetLockedObject = null;
		_carryingObject = null;
		_headTrackingObject = null;
		searching = false;
		lastActionRequested = RobotActionType.UNKNOWN;
		headAngleRequested = float.MaxValue;
		liftHeightRequested = float.MaxValue;
		headAngle_rad = float.MaxValue;
		poseAngle_rad = float.MaxValue;
		leftWheelSpeed_mmps = float.MaxValue;
		rightWheelSpeed_mmps = float.MaxValue;
		liftHeight_mm = float.MaxValue;
		batteryPercent = float.MaxValue;
		LastWorldPosition = Vector3.zero;
		WorldPosition = Vector3.zero;
		LastRotation = Quaternion.identity;
		Rotation = Quaternion.identity;
		localBusyTimer = 0f;
	}

	public void ClearObservedObjects()
	{
		for( int i = 0; i < observedObjects.Count; ++i )
		{
			if( observedObjects[i].TimeLastSeen + ObservedObject.RemoveDelay < Time.time )
			{
				observedObjects.RemoveAt( i-- );
			}
		}

		for( int i = 0; i < markersVisibleObjects.Count; ++i )
		{
			if( markersVisibleObjects[i].TimeLastSeen + ObservedObject.RemoveDelay < Time.time )
			{
				markersVisibleObjects.RemoveAt( i-- );
			}
		}
	}

	public void UpdateInfo( G2U.RobotState message )
	{
		headAngle_rad = message.headAngle_rad;
		poseAngle_rad = message.poseAngle_rad;
		leftWheelSpeed_mmps = message.leftWheelSpeed_mmps;
		rightWheelSpeed_mmps = message.rightWheelSpeed_mmps;
		liftHeight_mm = message.liftHeight_mm;
		status = (StatusFlag)message.status;
		batteryPercent = (message.batteryVoltage / MaxVoltage);
		carryingObjectID = message.carryingObjectID;
		headTrackingObjectID = message.headTrackingObjectID;

		if( headTrackingObjectID == lastHeadTrackingObjectID ) lastHeadTrackingObjectID = -1;

		LastWorldPosition = WorldPosition;
		WorldPosition = new Vector3(message.pose_x, message.pose_y,	message.pose_z);

		LastRotation = Rotation;
		Rotation = new Quaternion( message.pose_quaternion1, message.pose_quaternion2, message.pose_quaternion3, message.pose_quaternion0 );

	}

	public void UpdateLightMessages()
	{
		if( Time.time > ObservedObject.messageDelay )
		{
			for( int i = 0; i < knownObjects.Count; ++i )
			{
				if( knownObjects[i].Family == 3 && knownObjects[i].lightsChanged )
				{
					knownObjects[i].SetAllActiveObjectLEDs();
				}
			}
			ObservedObject.messageDelay = Time.time + GameController.MessageDelay;
		}
	}

	public void UpdateObservedObjectInfo( G2U.RobotObservedObject message )
	{
		if( message.objectFamily == 0 )
		{
			Debug.LogWarning( "UpdateObservedObjectInfo received message about the Mat!" );
			return;
		}

		ObservedObject knownObject = knownObjects.Find( x => x == message.objectID );

		//Debug.Log( "found ObservedObject ID(" + message.objectID +") objectType(" + message.objectType +")" );

		if( knownObject == null )
		{
			knownObject = new ObservedObject( message.objectID, message.objectFamily, message.objectType );

			knownObjects.Add( knownObject );
		}

		knownObject.UpdateInfo( message );

		if( observedObjects.Find( x => x == message.objectID ) == null )
		{
			observedObjects.Add( knownObject );
		}

		if( knownObject.MarkersVisible && markersVisibleObjects.Find( x => x == message.objectID ) == null )
		{
			markersVisibleObjects.Add( knownObject );
		}
	}

	public void DriveWheels( float leftWheelSpeedMmps, float rightWheelSpeedMmps )
	{
		//Debug.Log("DriveWheels(leftWheelSpeedMmps:"+leftWheelSpeedMmps+", rightWheelSpeedMmps:"+rightWheelSpeedMmps+")");
		U2G.DriveWheels message = new U2G.DriveWheels ();
		message.lwheel_speed_mmps = leftWheelSpeedMmps;
		message.rwheel_speed_mmps = rightWheelSpeedMmps;
		
		RobotEngineManager.instance.channel.Send( new U2G.Message { DriveWheels = message } );
	}

	public void PlaceObjectOnGroundHere()
	{
		Debug.Log( "Place Object On Ground Here" );
		
		U2G.PlaceObjectOnGroundHere message = new U2G.PlaceObjectOnGroundHere();
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ PlaceObjectOnGroundHere = message } );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	public void CancelAction( RobotActionType actionType = RobotActionType.UNKNOWN )
	{
		U2G.CancelAction message = new U2G.CancelAction();
		message.robotID = ID;
		message.actionType = actionType;

		Debug.Log( "CancelAction actionType("+actionType+")" );
		RobotEngineManager.instance.channel.Send( new U2G.Message { CancelAction = message } );
	}


	public void SetHeadAngle( float angle_rad = 0f )
	{
		if( headTrackingObject == -1 ) // if not head tracking, don't send same message as last
		{
			if( Mathf.Abs( angle_rad - headAngle_rad ) < 0.001f || Mathf.Abs( headAngleRequested - angle_rad ) < 0.001f )
			{
				return;
			}
		}
		else if( Time.time < lastHeadAngleRequestTime + CozmoUtil.LOCAL_BUSY_TIME ) // else, override head tracking after delay
		{
			return;
		}

		headAngleRequested = angle_rad;
		lastHeadAngleRequestTime = Time.time;

		Debug.Log( "Set Head Angle " + angle_rad );

		U2G.SetHeadAngle message = new U2G.SetHeadAngle();
		message.angle_rad = angle_rad;
		message.accel_rad_per_sec2 = 2f;
		message.max_speed_rad_per_sec = 5f;
		   
		RobotEngineManager.instance.channel.Send( new U2G.Message { SetHeadAngle = message } );
	}
	
	public void TrackHeadToObject( ObservedObject observedObject, bool faceObject = false )
	{
		if( headTrackingObjectID == observedObject )
		{
			lastHeadTrackingObjectID = -1;
			return;
		}
		else if( lastHeadTrackingObjectID == observedObject )
		{
			return;
		}

		lastHeadTrackingObjectID = observedObject;

		if( faceObject )
		{
			FaceObject( observedObject );
		}
		else
		{
			U2G.TrackHeadToObject message = new U2G.TrackHeadToObject();
			message.objectID = (uint)observedObject;
			message.robotID = ID;

			Debug.Log( "Track Head To Object " + message.objectID );
			
			RobotEngineManager.instance.channel.Send( new U2G.Message { TrackHeadToObject = message } );
		}
	}

	private void FaceObject( ObservedObject observedObject )
	{
		U2G.FaceObject message = new U2G.FaceObject();
		message.objectID = observedObject;
		message.robotID = ID;

		Debug.Log( "Face Object " + message.objectID );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message { FaceObject = message } );
	}

	public void PickAndPlaceObject( ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false )
	{
		U2G.PickAndPlaceObject message = new U2G.PickAndPlaceObject();
		message.objectID = selectedObject;
		message.usePreDockPose = System.Convert.ToByte( usePreDockPose );
		message.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		
		Debug.Log( "Pick And Place Object " + message.objectID + " usePreDockPose " + message.usePreDockPose + " useManualSpeed " + message.useManualSpeed );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ PickAndPlaceObject = message } );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	public void GotoPose( float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false )
	{
		U2G.GotoPose message = new U2G.GotoPose();
		message.level = System.Convert.ToByte( level );
		message.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		message.x_mm = x_mm;
		message.y_mm = y_mm;
		message.rad = rad;

		Debug.Log( "Go to Pose: x: " + message.x_mm + " y: " + message.y_mm + " useManualSpeed: " + message.useManualSpeed + " level: " + message.level );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ GotoPose = message } );
		
		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	public void SetLiftHeight( float height )
	{
		if(Time.time < lastLiftHeightRequestTime + CozmoUtil.LOCAL_BUSY_TIME && height == liftHeightRequested) return;

		if( liftHeight_mm == height )
		{
			return;
		}

		liftHeightRequested = height;
		lastLiftHeightRequestTime = Time.time;

		//Debug.Log( "Set Lift Height " + height );
		
		U2G.SetLiftHeight message = new U2G.SetLiftHeight();
		message.accel_rad_per_sec2 = 5f;
		message.max_speed_rad_per_sec = 10f;
		message.height_mm = height;
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetLiftHeight = message } );
	}
	
	public void SetRobotCarryingObject( int objectID = -1 )
	{
		Debug.Log( "Set Robot Carrying Object" );
		
		U2G.SetRobotCarryingObject message = new U2G.SetRobotCarryingObject();
		
		message.robotID = ID;
		message.objectID = objectID;
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetRobotCarryingObject = message } );
		selectedObjects.Clear();
		targetLockedObject = null;
		
		SetLiftHeight( 0f );
		SetHeadAngle();
	}
	
	public void ClearAllBlocks()
	{
		Debug.Log( "Clear All Blocks" );
		
		U2G.ClearAllBlocks message = new U2G.ClearAllBlocks();
		
		RobotEngineManager.instance.channel.Send( new U2G.Message { ClearAllBlocks = message } );
		Reset();
		
		SetLiftHeight( 0f );
		SetHeadAngle();
	}
	
	public void VisionWhileMoving( bool enable )
	{
		Debug.Log( "Vision While Moving " + enable );
		
		U2G.VisionWhileMoving message = new U2G.VisionWhileMoving();
		message.enable = System.Convert.ToByte( enable );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message { VisionWhileMoving = message } );
	}
	
	public void RequestImage( RobotEngineManager.CameraResolution resolution, RobotEngineManager.ImageSendMode_t mode )
	{
		U2G.SetRobotImageSendMode message = new U2G.SetRobotImageSendMode ();
		message.resolution = (byte)resolution;
		message.mode = (byte)mode;
		
		RobotEngineManager.instance.channel.Send( new U2G.Message {SetRobotImageSendMode = message});
		
		U2G.ImageRequest message2 = new U2G.ImageRequest();
		message2.robotID = ID;
		message2.mode = (byte)mode;
		
		RobotEngineManager.instance.channel.Send( new U2G.Message { ImageRequest = message2 } );
		
		Debug.Log( "image request message sent with " + mode + " at " + resolution );
	}
	
	public void StopAllMotors()
	{
		U2G.StopAllMotors message = new U2G.StopAllMotors ();
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ StopAllMotors = message } );
	}
	
	public void TurnInPlace( float angle_rad )
	{
		U2G.TurnInPlace message = new U2G.TurnInPlace ();
		message.robotID = ID;
		message.angle_rad = angle_rad;
		
		Debug.Log( "TurnInPlace(robotID:" + ID + ", angle_rad:" + angle_rad + ")" );
		RobotEngineManager.instance.channel.Send( new U2G.Message{ TurnInPlace = message } );
	}

	public void TraverseObject( int objectID, bool usePreDockPose = false, bool useManualSpeed = false )
	{
		Debug.Log( "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose );
		
		U2G.TraverseObject message = new U2G.TraverseObject();
		message.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		message.usePreDockPose = System.Convert.ToByte( usePreDockPose );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ TraverseObject = message } );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	private bool IsNotInRange(ObservedObject obj)
	{
		return !((obj.WorldPosition - WorldPosition).sqrMagnitude <= objectPertinenceRange * objectPertinenceRange);
	}
	
	private bool IsNotInZRange(ObservedObject obj)
	{
		return !(obj.WorldPosition.z - WorldPosition.z <= Z_RANGE_LIMIT);
	}
	
	private int SortByDistance(ObservedObject a, ObservedObject b)
	{
		float d1 = ( (Vector2)a.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
		float d2 = ( (Vector2)b.WorldPosition - (Vector2)WorldPosition).sqrMagnitude;
		return d1.CompareTo(d2);
	}
}
