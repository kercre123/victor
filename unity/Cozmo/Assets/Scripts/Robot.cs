using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class Robot
{
	public byte ID { get; private set; }
	public float headAngle_rad { get; private set; }
	public float poseAngle_rad { get; private set; }
	public float leftWheelSpeed_mmps { get; private set; }
	public float rightWheelSpeed_mmps { get; private set; }
	public float liftHeight_mm { get; private set; }
	public float pose_x { get; private set; }
	public float pose_y { get; private set; }
	public float pose_z { get; private set; }
	public List<ObservedObject> observedObjects { get; private set; }
	public uint selectedObject;

	public Robot( byte robotID )
	{
		ID = robotID;
		selectedObject = uint.MaxValue;
		observedObjects = new List<ObservedObject>();
	}

	public void UpdateInfo( G2U_RobotState message )
	{
		headAngle_rad = message.headAngle_rad;
		poseAngle_rad = message.poseAngle_rad;
		leftWheelSpeed_mmps = message.leftWheelSpeed_mmps;
		rightWheelSpeed_mmps = message.rightWheelSpeed_mmps;
		liftHeight_mm = message.liftHeight_mm;
		pose_x = message.pose_x;
		pose_y = message.pose_y;
		pose_z = message.pose_z;
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
	}
}
