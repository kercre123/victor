using UnityEngine;
using System.Collections;
using System.Collections.Generic;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;


//Red, Orange, Yellow, Green, Blue, Purple
public enum ActiveBlockType {
	Off,
	White,
	Red,
	Orange,
	Yellow,
	Green,
	Blue,
	Purple,
	NumTypes
}

[System.FlagsAttribute]
public enum Light
{
	NONE              = 0,
	TOP_NORTH_WEST    = 0x1,
	TOP_NORTH_EAST    = 0x2,
	TOP_SOUTH_EAST    = 0x4,
	TOP_SOUTH_WEST    = 0x8,
	BOTTOM_NORTH_WEST = 0x10,
	BOTTOM_NORTH_EAST = 0x20,
	BOTTOM_SOUTH_EAST = 0x40,
	BOTTOM_SOUTH_WEST = 0x80,
	ALL = 0xff
};

public class ObservedObject
{
	public uint RobotID { get; private set; }
	public uint Family { get; private set; }
	public uint ObjectType { get; private set; }

	private int ID;

	public bool MarkersVisible { get { return Time.time - TimeLastSeen < 0.5f; } }

	public Rect VizRect { get; private set; }
	public Vector3 WorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }
	public Vector3 Forward { get { return Rotation * Vector3.right;	} }
	public Vector3 Right { get { return Rotation * -Vector3.up;	} }

	public Vector3 Size { get; private set; }
	public float TimeLastSeen { get; private set; }
	public float TimeCreated { get; private set; }

	public uint Color { get; private set; }

	public ActiveBlockType activeBlockType = ActiveBlockType.Off;

	public const float RemoveDelay = 0.15f;

	public float Distance { get { return Vector2.Distance( RobotEngineManager.instance.current.WorldPosition, WorldPosition ); } }

	public ObservedObject()
	{
		TimeCreated = Time.time;
	}

	public static implicit operator uint( ObservedObject observedObject )
	{
		if( observedObject == null )
		{
			Debug.LogWarning( "converting null ObservedObject into uint: returning uint.MaxValue" );
			return uint.MaxValue;
		}
		
		return (uint)observedObject.ID;
	}

	public static implicit operator int( ObservedObject observedObject )
	{
		if( observedObject == null ) return -1;

		return observedObject.ID;
	}

	public static implicit operator string( ObservedObject observedObject )
	{
		return ((int)observedObject).ToString();
	}
	
	public void UpdateInfo( G2U.RobotObservedObject message )
	{
		RobotID = message.robotID;
		Family = message.objectFamily;
		ObjectType = message.objectType;
		ID = message.objectID;
		VizRect = new Rect( message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height );

		//dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
		WorldPosition = new Vector3( message.world_x, message.world_y, message.world_z );
		Rotation = new Quaternion( message.quaternion1, message.quaternion2, message.quaternion3, message.quaternion0 );
		Size = Vector3.one * CozmoUtil.BLOCK_LENGTH_MM;

		if( message.markersVisible > 0 ) TimeLastSeen = Time.time;
	}

	public void SendLightMessageRelativeToRobot( float light_intensity, uint color = 0, byte whichLEDs = byte.MaxValue, 
	                             uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                             uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                             byte turnOffUnspecifiedLEDs = 1 )
	{
		U2G.SetActiveObjectLEDs message = new U2G.SetActiveObjectLEDs ();
		message.objectID = (uint)ID;
		message.robotID = (byte)RobotID;
		message.onPeriod_ms = onPeriod_ms;
		message.offPeriod_ms = offPeriod_ms;
		message.transitionOnPeriod_ms = transitionOnPeriod_ms;
		message.transitionOffPeriod_ms = transitionOffPeriod_ms;
		message.turnOffUnspecifiedLEDs = turnOffUnspecifiedLEDs;
		
		message.color = color;
		Color = color;
		
		message.whichLEDs = whichLEDs;
		message.makeRelative = 1;
		message.relativeToX = RobotEngineManager.instance.robots[(int)RobotID].WorldPosition.x;
		message.relativeToY = RobotEngineManager.instance.robots[(int)RobotID].WorldPosition.y;
		
		Debug.Log( "SendLightMessage: color" + message.color + " onPeriod_ms: " + onPeriod_ms + " offPeriod_ms: " + offPeriod_ms );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetActiveObjectLEDs = message } );
	}

	public void SendLightMessage( float light_intensity, uint color = 0, byte whichLEDs = byte.MaxValue, 
	                             uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                             uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                             byte turnOffUnspecifiedLEDs = 1 )
	{
		U2G.SetActiveObjectLEDs message = new U2G.SetActiveObjectLEDs ();
		message.objectID = (uint)ID;
		message.robotID = (byte)RobotID;
		message.onPeriod_ms = onPeriod_ms;
		message.offPeriod_ms = offPeriod_ms;
		message.transitionOnPeriod_ms = transitionOnPeriod_ms;
		message.transitionOffPeriod_ms = transitionOffPeriod_ms;
		message.turnOffUnspecifiedLEDs = turnOffUnspecifiedLEDs;
		
		message.color = color;
		Color = color;
		
		message.whichLEDs = whichLEDs;
		message.makeRelative = 0;
		message.relativeToX = 0;
		message.relativeToY = 0;

		Debug.Log( "SendLightMessage: color" + message.color + " onPeriod_ms: " + onPeriod_ms + " offPeriod_ms: " + offPeriod_ms );

		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetActiveObjectLEDs = message } );
	}

	public void SendLightMessageRelative( float light_intensity, Vector2 relativeTo, uint color = 0, byte whichLEDs = byte.MaxValue, 
	                                     uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                                     uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                                     byte turnOffUnspecifiedLEDs = 1 )
	{
		U2G.SetActiveObjectLEDs message = new U2G.SetActiveObjectLEDs();
		message.objectID = (uint)ID;
		message.robotID = RobotEngineManager.instance.current.ID;
		message.onPeriod_ms = onPeriod_ms;
		message.offPeriod_ms = offPeriod_ms;
		message.transitionOnPeriod_ms = transitionOnPeriod_ms;
		message.transitionOffPeriod_ms = transitionOffPeriod_ms;
		message.turnOffUnspecifiedLEDs = turnOffUnspecifiedLEDs;
		
		message.color = color;
		Color = color;
		
		message.whichLEDs = whichLEDs;
		message.makeRelative = 1;
		message.relativeToX = relativeTo.x;
		message.relativeToY = relativeTo.y;
		
		Debug.Log( "SendLightMessage: color" + message.color + " onPeriod_ms: " + onPeriod_ms + " offPeriod_ms: " + offPeriod_ms );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetActiveObjectLEDs = message } );
	}
}
