using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;

public class ObservedObject
{
	public uint RobotID { get; private set; }
	public uint Family { get; private set; }
	public uint ObjectType { get; private set; }
	public int ID { get; private set; }

	bool _markersVisible = false;
	public bool MarkersVisible { 
		get {
			return _markersVisible && (Time.time - TimeLastSeen) < 0.5f;

		}
		private set {
			_markersVisible = value;
		}
	}

	public Rect VizRect { get; private set; }
	public Vector3 WorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }
	public Vector3 Forward { get { return Rotation * Vector3.right;	} }
	public Vector3 Right { get { return Rotation * -Vector3.up;	} }

	public Vector3 Size { get; private set; }
	public float TimeLastSeen { get; private set; }

	public const float RemoveDelay = 0.15f;

	public float Distance
	{ 
		get
		{
			return Vector2.Distance( RobotEngineManager.instance.current.WorldPosition, WorldPosition );
		}
	}

	public void UpdateInfo( G2U_RobotObservedObject message )
	{
		RobotID = message.robotID;
		Family = message.objectFamily;
		ObjectType = message.objectType;
		ID = message.objectID;
		MarkersVisible = System.Convert.ToBoolean( message.markersVisible );
		VizRect = new Rect( message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height );

		//dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
		WorldPosition = new Vector3( message.world_x, message.world_y, message.world_z );
		Rotation = new Quaternion( message.quaternion1, message.quaternion2, message.quaternion3, message.quaternion0 );
		Size = new Vector3( 44f, 44f, 44f );
		TimeLastSeen = Time.time;
	}
}
