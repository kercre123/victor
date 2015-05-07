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
	Yellow,
	Green,
	Cyan,
	Blue,
	Magenta,
	NumTypes
}

public class Light
{
	[System.FlagsAttribute]
	public enum PositionFlag
	{
		NONE              = 0,
		TOP_NORTH_WEST    = 0x01,
		TOP_NORTH_EAST    = 0x10,
		TOP_SOUTH_EAST    = 0x02,
		TOP_SOUTH_WEST    = 0x20,
		BOTTOM_NORTH_WEST = 0x04,
		BOTTOM_NORTH_EAST = 0x40,
		BOTTOM_SOUTH_EAST = 0x08,
		BOTTOM_SOUTH_WEST = 0x80,
		ALL = 0xff
	};

	private static PositionFlag IndexToPosition( int i )
	{
		switch( i )
		{
			case 0:
				return PositionFlag.TOP_NORTH_WEST;
			case 1:
				return PositionFlag.TOP_SOUTH_EAST;
			case 2:
				return PositionFlag.BOTTOM_NORTH_WEST;
			case 3:
				return PositionFlag.BOTTOM_SOUTH_EAST;
			case 4:
				return PositionFlag.TOP_NORTH_EAST;
			case 5:
				return PositionFlag.TOP_SOUTH_WEST;
			case 6:
				return PositionFlag.BOTTOM_NORTH_EAST;
			case 7:
				return PositionFlag.BOTTOM_SOUTH_WEST;
			case 8:
				return PositionFlag.ALL;
		}

		return PositionFlag.NONE;
	}

	private ObservedObject observedObject;
	private PositionFlag position;

	private uint lastOnColor;
	public uint onColor;
	private uint lastOffColor;
	public uint offColor;
	private uint lastOnPeriod_ms;
	public uint onPeriod_ms;
	private uint lastOffPeriod_ms;
	public uint offPeriod_ms;
	private uint lastTransitionOnPeriod_ms;
	public uint transitionOnPeriod_ms;
	private uint lastTransitionOffPeriod_ms;
	public uint transitionOffPeriod_ms;

	public Light( ObservedObject observedObject, int position )
	{
		this.observedObject = observedObject;
		this.position = IndexToPosition( position );
	}

	public bool Position( PositionFlag s )
	{
		return (position | s) == s;
	}

	public void SetLastInfo()
	{
		lastOnColor = onColor;
		lastOffColor = offColor;
		lastOnPeriod_ms = onPeriod_ms;
		lastOffPeriod_ms = offPeriod_ms;
		lastTransitionOnPeriod_ms = transitionOnPeriod_ms;
		lastTransitionOffPeriod_ms = transitionOffPeriod_ms;
	}

	public bool changed
	{
		get
		{
			return lastOnColor != onColor || lastOffColor != offColor || lastOnPeriod_ms != onPeriod_ms || lastOffPeriod_ms != offPeriod_ms || 
				lastTransitionOnPeriod_ms != transitionOnPeriod_ms || lastTransitionOffPeriod_ms != transitionOffPeriod_ms;
		}
	}
}

public class ObservedObject
{
	public uint RobotID { get; private set; }
	public uint Family { get; private set; }
	public uint ObjectType { get; private set; }

	private int ID;
	private U2G.SetAllActiveObjectLEDs message;

	public bool MarkersVisible { get { return Time.time - TimeLastSeen < 0.5f; } }

	public Rect VizRect { get; private set; }
	public Vector3 WorldPosition { get; private set; }
	public Quaternion Rotation { get; private set; }
	public Vector3 Forward { get { return Rotation * Vector3.right;	} }
	public Vector3 Right { get { return Rotation * -Vector3.up;	} }

	public Vector3 Size { get; private set; }
	public float TimeLastSeen { get; private set; }
	public float TimeCreated { get; private set; }

	private Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	public uint Color { get; private set; }

	public Light[] lights { get; private set; }

	public bool lightsChanged
	{
		get
		{
			if( lastRelativeMode != relativeMode || lastRelativeToX != relativeToX || lastRelativeToY != relativeToY ) return true;

			for( int i = 0; i < lights.Length; ++i )
			{
				if( lights[i].changed ) return true;
			}

			return false;
		}
	}

	public bool canBeStackedOn
	{
		get
		{
			if( robot.selectedObjects.Count > 1 ) return false; // if blocks stacked on each other

			float distance = ( (Vector2)WorldPosition - (Vector2)robot.WorldPosition ).magnitude;
			float height = Mathf.Abs( WorldPosition.z - robot.WorldPosition.z );
		
			return distance <= CozmoUtil.BLOCK_LENGTH_MM * 4f && height < CozmoUtil.BLOCK_LENGTH_MM;
		}
	}

	private byte lastRelativeMode;
	public byte relativeMode;

	private float lastRelativeToX;
	public float relativeToX;
	private float lastRelativeToY;
	public float relativeToY;

	public ActiveBlockType activeBlockType = ActiveBlockType.Off;

	public const float RemoveDelay = 0.15f;

	public static float messageDelay = 0f;

	public float Distance { get { return Vector2.Distance( RobotEngineManager.instance.current.WorldPosition, WorldPosition ); } }

	public ObservedObject( int objectID, uint objectFamily, uint objectType )
	{
		TimeCreated = Time.time;
		Family = objectFamily;
		ObjectType = objectType;
		ID = objectID;

		if( Family == 3 )
		{
			lights = new Light[8];

			message = new U2G.SetAllActiveObjectLEDs();
			message.onPeriod_ms = new uint[lights.Length];
			message.offPeriod_ms = new uint[lights.Length];
			message.transitionOnPeriod_ms = new uint[lights.Length];
			message.transitionOffPeriod_ms = new uint[lights.Length];
			message.onColor = new uint[lights.Length];
			message.offColor = new uint[lights.Length];

			for( int i = 0; i < lights.Length; ++i )
			{
				lights[i] = new Light( this, i );
			}
		}
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
		VizRect = new Rect( message.img_topLeft_x, message.img_topLeft_y, message.img_width, message.img_height );

		//dmdnote cozmo's space is Z up, keep in mind if we need to convert to unity's y up space.
		WorldPosition = new Vector3( message.world_x, message.world_y, message.world_z );
		Rotation = new Quaternion( message.quaternion1, message.quaternion2, message.quaternion3, message.quaternion0 );
		Size = Vector3.one * CozmoUtil.BLOCK_LENGTH_MM;

		if( message.markersVisible > 0 ) TimeLastSeen = Time.time;
	}

	public void SetAllActiveObjectLEDs() // should only be called from update loop
	{
		if( Family != 3 )
		{
			Debug.LogWarning( "Cannot send light message for non active block " + ID );
			return;
		}

		message.objectID = (uint)ID;
		message.robotID = (byte)RobotID;

		for( int i = 0; i < lights.Length; ++i )
		{
			message.onPeriod_ms[i] = lights[i].onPeriod_ms;
			message.offPeriod_ms[i] = lights[i].offPeriod_ms;
			message.transitionOnPeriod_ms[i] = lights[i].transitionOnPeriod_ms;
			message.transitionOffPeriod_ms[i] = lights[i].transitionOffPeriod_ms;
			message.onColor[i] = lights[i].onColor;
			message.offColor[i] = lights[i].offColor;
		}

		message.makeRelative = relativeMode;
		message.relativeToX = relativeToX;
		message.relativeToY = relativeToY;

		Debug.Log( "SetAllActiveObjectLEDs for Object with ID: " + ID );
		
		RobotEngineManager.instance.channel.Send( new U2G.Message{ SetAllActiveObjectLEDs = message } );

		SetLastActiveObjectLEDs();
	}

	private void SetLastActiveObjectLEDs()
	{
		lastRelativeMode = relativeMode;
		lastRelativeToX = relativeToX;
		lastRelativeToY = relativeToY;

		for( int i = 0; i < lights.Length; ++i )
		{
			lights[i].SetLastInfo();
		}
	}

	public void SetActiveObjectLEDs( uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, 
	                             uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                             uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                             byte turnOffUnspecifiedLEDs = 1 )
	{
		if( Family != 3 )
		{
			Debug.LogWarning( "Cannot send light message for non active block " + ID );
			return;
		}

		this.Color = onColor;

		for( int i = 0; i < lights.Length; ++i )
		{
			if( lights[i].Position( (Light.PositionFlag)whichLEDs ) )
			{
				lights[i].onColor = onColor;
				lights[i].offColor = offColor;
				lights[i].onPeriod_ms = onPeriod_ms;
				lights[i].offPeriod_ms = offPeriod_ms;
				lights[i].transitionOnPeriod_ms = transitionOnPeriod_ms;
				lights[i].transitionOffPeriod_ms = transitionOffPeriod_ms;
			}
			else if( turnOffUnspecifiedLEDs > 0 )
			{
				lights[i].onColor = 0;
				lights[i].offColor = 0;
				lights[i].onPeriod_ms = 0;
				lights[i].offPeriod_ms = 0;
				lights[i].transitionOnPeriod_ms = 0;
				lights[i].transitionOffPeriod_ms = 0;
			}
		}

		relativeMode = 0;
		relativeToX = 0;
		relativeToY = 0;
	}

	public void SetActiveObjectLEDsRelative( Vector2 relativeTo, uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, byte relativeMode = 1,
	                                        uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                                        uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                                        byte turnOffUnspecifiedLEDs = 1 )
	{	
		SetActiveObjectLEDsRelative( relativeTo.x, relativeTo.y, onColor, offColor, whichLEDs, relativeMode, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs );
	}

	public void SetActiveObjectLEDsRelative( float relativeToX, float relativeToY, uint onColor = 0, uint offColor = 0, byte whichLEDs = byte.MaxValue, byte relativeMode = 1,
	                                     uint onPeriod_ms = 1000, uint offPeriod_ms = 0,
	                                     uint transitionOnPeriod_ms = 0, uint transitionOffPeriod_ms = 0,
	                                     byte turnOffUnspecifiedLEDs = 1 )
	{
		if( Family != 3 )
		{
			Debug.LogWarning( "Cannot send light message for non active block " + ID );
			return;
		}

		SetActiveObjectLEDs( onColor, offColor, whichLEDs, onPeriod_ms, offPeriod_ms, transitionOnPeriod_ms, transitionOffPeriod_ms, turnOffUnspecifiedLEDs );

		this.relativeMode = relativeMode;
		this.relativeToX = relativeToX;
		this.relativeToY = relativeToY;
	}
}
