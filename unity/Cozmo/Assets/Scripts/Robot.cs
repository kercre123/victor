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

	public float batteryPercent { get; private set; }
	public int carryingObjectID { get; private set; }
	public List<ObservedObject> observedObjects { get; private set; }
	public List<ObservedObject> knownObjects { get; private set; }
	public List<int> selectedObjects { get; private set; }
	public List<int> lastSelectedObjects { get; private set; }
	public int lastObjectHeadTracked;

	// er, should be 5?
	private const float MaxVoltage = 5.0f;

	private StatusFlag status;
	private StatusFlag lastStatus;

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
		selectedObjects = new List<int>();
		lastSelectedObjects = new List<int>();
		lastObjectHeadTracked = -1;
		observedObjects = new List<ObservedObject>();
		knownObjects = new List<ObservedObject>();

		RobotEngineManager.instance.DisconnectedFromClient += Reset;
	}

	public void Reset( DisconnectionReason reason = DisconnectionReason.None )
	{
		selectedObjects.Clear();
		lastSelectedObjects.Clear();
		lastObjectHeadTracked = -1;
		observedObjects.Clear();
		knownObjects.Clear();
		lastStatus = StatusFlag.NONE;
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

		//stubbing in rot until we have it sent over
		Rotation = Quaternion.identity;

		if( status != lastStatus )
		{
			RobotEngineManager.instance.statusText.text = "Status: " + status;
			Debug.Log( RobotEngineManager.instance.statusText.text );
			lastStatus = status;
		}
	}

	public void UpdateObservedObjectInfo( G2U_RobotObservedObject message )
	{
		ObservedObject observedObject = observedObjects.Find( x => x.ID == message.objectID );

		if( observedObject == null )
		{
			observedObject = new ObservedObject();

			observedObjects.Add( observedObject );
		}

		observedObject.UpdateInfo( message );

		ObservedObject knownObject = knownObjects.Find( x => x.ID == message.objectID );

		if(knownObject == null)
		{
			knownObjects.Add( observedObject );
		}
	}
}
