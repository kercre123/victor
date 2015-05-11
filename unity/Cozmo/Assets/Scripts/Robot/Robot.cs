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
	public Dictionary<int, ActiveBlock> activeBlocks { get; private set; }

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

	private U2G.DriveWheels DriveWheelsMessage;
	private U2G.PlaceObjectOnGroundHere PlaceObjectOnGroundHereMessage;
	private U2G.CancelAction CancelActionMessage;
	private U2G.SetHeadAngle SetHeadAngleMessage;
	private U2G.TrackHeadToObject TrackHeadToObjectMessage;
	private U2G.FaceObject FaceObjectMessage;
	private U2G.PickAndPlaceObject PickAndPlaceObjectMessage;
	private U2G.GotoPose GotoPoseMessage;
	private U2G.SetLiftHeight SetLiftHeightMessage;
	private U2G.SetRobotCarryingObject SetRobotCarryingObjectMessage;
	private U2G.ClearAllBlocks ClearAllBlocksMessage;
	private U2G.VisionWhileMoving VisionWhileMovingMessage;
	private U2G.SetRobotImageSendMode SetRobotImageSendModeMessage;
	private U2G.ImageRequest ImageRequestMessage;
	private U2G.StopAllMotors StopAllMotorsMessage;
	private U2G.TurnInPlace TurnInPlaceMessage;
	private U2G.TraverseObject TraverseObjectMessage;

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
	[System.NonSerialized] public bool localBusyOverride = false;
	public bool isBusy
	{
		get
		{
			return localBusyOverride
				|| localBusyTimer > 0f 
				|| Status( StatusFlag.IS_PERFORMING_ACTION )
				|| Status( StatusFlag.IS_ANIMATING ) 
				|| Status( StatusFlag.IS_PICKED_UP );
		}

		set
		{
			localBusyOverride = value;

			if( value )
			{
				DriveWheels(0,0); 
			}

			Debug.Log("isBusy = " + value);
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
		activeBlocks = new Dictionary<int, ActiveBlock>();

		DriveWheelsMessage = new U2G.DriveWheels();
		PlaceObjectOnGroundHereMessage = new U2G.PlaceObjectOnGroundHere();
		CancelActionMessage = new U2G.CancelAction();
		SetHeadAngleMessage = new U2G.SetHeadAngle();
		TrackHeadToObjectMessage = new U2G.TrackHeadToObject();
		FaceObjectMessage = new U2G.FaceObject();
		PickAndPlaceObjectMessage = new U2G.PickAndPlaceObject();
		GotoPoseMessage = new U2G.GotoPose();
		SetLiftHeightMessage = new U2G.SetLiftHeight();
		SetRobotCarryingObjectMessage = new U2G.SetRobotCarryingObject();
		ClearAllBlocksMessage = new U2G.ClearAllBlocks();
		VisionWhileMovingMessage = new U2G.VisionWhileMoving();
		SetRobotImageSendModeMessage = new U2G.SetRobotImageSendMode();
		ImageRequestMessage = new U2G.ImageRequest();
		StopAllMotorsMessage = new U2G.StopAllMotors();
		TurnInPlaceMessage = new U2G.TurnInPlace();
		TraverseObjectMessage = new U2G.TraverseObject();

		ClearData();

		RobotEngineManager.instance.DisconnectedFromClient += Reset;

		RefreshObjectPertinence();
	}

	public void CooldownTimers(float delta) {
		if(localBusyTimer > 0f) {
			localBusyTimer -= delta;
		}
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
		activeBlocks.Clear();
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
		if( Time.time > ActiveBlock.messageDelay )
		{
			foreach( ActiveBlock activeBlock in activeBlocks.Values )
			{
				if( activeBlock.lightsChanged )
				{
					activeBlock.SetAllLEDs();
				}
			}
		}
	}

	public void UpdateObservedObjectInfo( G2U.RobotObservedObject message )
	{
		if( message.objectFamily == 0 )
		{
			Debug.LogWarning( "UpdateObservedObjectInfo received message about the Mat!" );
			return;
		}

		if( message.objectFamily == 3 )
		{
			AddActiveBlock( activeBlocks.ContainsKey( message.objectID ) ? activeBlocks[ message.objectID ] : null, message );
		}
		else
		{
			ObservedObject knownObject = knownObjects.Find( x => x == message.objectID );

			AddObservedObject( knownObject, message );
		}
	}

	private void AddActiveBlock( ActiveBlock activeBlock, G2U.RobotObservedObject message )
	{
		if( activeBlock == null )
		{
			activeBlock = new ActiveBlock( message.objectID, message.objectFamily, message.objectType );

			activeBlocks.Add( activeBlock, activeBlock );
			knownObjects.Add( activeBlock );
		}

		AddObservedObject( activeBlock, message );
	}
	
	private void AddObservedObject( ObservedObject knownObject, G2U.RobotObservedObject message )
	{
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
		DriveWheelsMessage.lwheel_speed_mmps = leftWheelSpeedMmps;
		DriveWheelsMessage.rwheel_speed_mmps = rightWheelSpeedMmps;

		RobotEngineManager.instance.Message.DriveWheels = DriveWheelsMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}

	public void PlaceObjectOnGroundHere()
	{
		Debug.Log( "Place Object " + carryingObject + " On Ground Here" );

		RobotEngineManager.instance.Message.PlaceObjectOnGroundHere = PlaceObjectOnGroundHereMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	public void CancelAction( RobotActionType actionType = RobotActionType.UNKNOWN )
	{
		CancelActionMessage.robotID = ID;
		CancelActionMessage.actionType = actionType;

		Debug.Log( "CancelAction actionType("+actionType+")" );

		RobotEngineManager.instance.Message.CancelAction = CancelActionMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
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

		SetHeadAngleMessage.angle_rad = angle_rad;
		SetHeadAngleMessage.accel_rad_per_sec2 = 2f;
		SetHeadAngleMessage.max_speed_rad_per_sec = 5f;

		RobotEngineManager.instance.Message.SetHeadAngle = SetHeadAngleMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
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
			TrackHeadToObjectMessage.objectID = (uint)observedObject;
			TrackHeadToObjectMessage.robotID = ID;

			Debug.Log( "Track Head To Object " + TrackHeadToObjectMessage.objectID );

			RobotEngineManager.instance.Message.TrackHeadToObject = TrackHeadToObjectMessage;
			RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
		}
	}

	private void FaceObject( ObservedObject observedObject )
	{
		FaceObjectMessage.objectID = observedObject;
		FaceObjectMessage.robotID = ID;

		Debug.Log( "Face Object " + FaceObjectMessage.objectID );

		RobotEngineManager.instance.Message.FaceObject = FaceObjectMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}

	public void PickAndPlaceObject( ObservedObject selectedObject, bool usePreDockPose = true, bool useManualSpeed = false )
	{
		PickAndPlaceObjectMessage .objectID = selectedObject;
		PickAndPlaceObjectMessage.usePreDockPose = System.Convert.ToByte( usePreDockPose );
		PickAndPlaceObjectMessage.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		
		Debug.Log( "Pick And Place Object " + PickAndPlaceObjectMessage.objectID + " usePreDockPose " + PickAndPlaceObjectMessage.usePreDockPose + " useManualSpeed " + PickAndPlaceObjectMessage.useManualSpeed );

		RobotEngineManager.instance.Message.PickAndPlaceObject = PickAndPlaceObjectMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}

	public void GotoPose( float x_mm, float y_mm, float rad, bool level = false, bool useManualSpeed = false )
	{
		GotoPoseMessage.level = System.Convert.ToByte( level );
		GotoPoseMessage.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		GotoPoseMessage.x_mm = x_mm;
		GotoPoseMessage.y_mm = y_mm;
		GotoPoseMessage.rad = rad;

		Debug.Log( "Go to Pose: x: " + GotoPoseMessage.x_mm + " y: " + GotoPoseMessage.y_mm + " useManualSpeed: " + GotoPoseMessage.useManualSpeed + " level: " + GotoPoseMessage.level );

		RobotEngineManager.instance.Message.GotoPose = GotoPoseMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
		
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

		SetLiftHeightMessage.accel_rad_per_sec2 = 5f;
		SetLiftHeightMessage.max_speed_rad_per_sec = 10f;
		SetLiftHeightMessage.height_mm = height;

		RobotEngineManager.instance.Message.SetLiftHeight = SetLiftHeightMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}
	
	public void SetRobotCarryingObject( int objectID = -1 )
	{
		Debug.Log( "Set Robot Carrying Object" );
		
		SetRobotCarryingObjectMessage.robotID = ID;
		SetRobotCarryingObjectMessage.objectID = objectID;

		RobotEngineManager.instance.Message.SetRobotCarryingObject = SetRobotCarryingObjectMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
		selectedObjects.Clear();
		targetLockedObject = null;
		
		SetLiftHeight( 0f );
		SetHeadAngle();
	}
	
	public void ClearAllBlocks()
	{
		Debug.Log( "Clear All Blocks" );

		RobotEngineManager.instance.Message.ClearAllBlocks = ClearAllBlocksMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
		Reset();
		
		SetLiftHeight( 0f );
		SetHeadAngle();
	}
	
	public void VisionWhileMoving( bool enable )
	{
		Debug.Log( "Vision While Moving " + enable );

		VisionWhileMovingMessage.enable = System.Convert.ToByte( enable );

		RobotEngineManager.instance.Message.VisionWhileMoving = VisionWhileMovingMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}
	
	public void RequestImage( RobotEngineManager.CameraResolution resolution, RobotEngineManager.ImageSendMode_t mode )
	{
		SetRobotImageSendModeMessage.resolution = (byte)resolution;
		SetRobotImageSendModeMessage.mode = (byte)mode;

		RobotEngineManager.instance.Message.SetRobotImageSendMode = SetRobotImageSendModeMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );

		ImageRequestMessage.robotID = ID;
		ImageRequestMessage.mode = (byte)mode;

		RobotEngineManager.instance.Message.ImageRequest = ImageRequestMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
		
		Debug.Log( "image request message sent with " + mode + " at " + resolution );
	}
	
	public void StopAllMotors()
	{
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}
	
	public void TurnInPlace( float angle_rad )
	{
		TurnInPlaceMessage.robotID = ID;
		TurnInPlaceMessage.angle_rad = angle_rad;
		
		Debug.Log( "TurnInPlace(robotID:" + ID + ", angle_rad:" + angle_rad + ")" );

		RobotEngineManager.instance.Message.TurnInPlace = TurnInPlaceMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );
	}

	public void TraverseObject( int objectID, bool usePreDockPose = false, bool useManualSpeed = false )
	{
		Debug.Log( "Traverse Object " + objectID + " useManualSpeed " + useManualSpeed + " usePreDockPose " + usePreDockPose );

		TraverseObjectMessage.useManualSpeed = System.Convert.ToByte( useManualSpeed );
		TraverseObjectMessage.usePreDockPose = System.Convert.ToByte( usePreDockPose );

		RobotEngineManager.instance.Message.TraverseObject = TraverseObjectMessage;
		RobotEngineManager.instance.channel.Send( RobotEngineManager.instance.Message );

		localBusyTimer = CozmoUtil.LOCAL_BUSY_TIME;
	}
}
