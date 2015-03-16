namespace Anki.Cozmo
{
public class G2U_Ping
{
	public uint counter;

	/**** Constructors ****/

	public G2U_Ping()
	{
	}

	public G2U_Ping(uint counter)
	{
		this.counter = counter;
	}

	public G2U_Ping(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//counter
		counter = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)counter);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//counter
		counter = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//counter
			result += 4; // = uint_32
			return result;
		}
	}
}

public class G2U_RobotAvailable
{
	public uint robotID;

	/**** Constructors ****/

	public G2U_RobotAvailable()
	{
	}

	public G2U_RobotAvailable(uint robotID)
	{
		this.robotID = robotID;
	}

	public G2U_RobotAvailable(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			return result;
		}
	}
}

public class G2U_UiDeviceAvailable
{
	public uint deviceID;

	/**** Constructors ****/

	public G2U_UiDeviceAvailable()
	{
	}

	public G2U_UiDeviceAvailable(uint deviceID)
	{
		this.deviceID = deviceID;
	}

	public G2U_UiDeviceAvailable(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)deviceID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//deviceID
			result += 4; // = uint_32
			return result;
		}
	}
}

public class G2U_RobotConnected
{
	public uint robotID;
	public byte successful;

	/**** Constructors ****/

	public G2U_RobotConnected()
	{
	}

	public G2U_RobotConnected(uint robotID,
		byte successful)
	{
		this.robotID = robotID;
		this.successful = successful;
	}

	public G2U_RobotConnected(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//successful
		successful = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		writer.Write((byte)successful);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//successful
		successful = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			//successful
			result += 1; // = uint_8
			return result;
		}
	}
}

public class G2U_RobotDisconnected
{
	public uint robotID;
	public float timeSinceLastMsg_sec;

	/**** Constructors ****/

	public G2U_RobotDisconnected()
	{
	}

	public G2U_RobotDisconnected(uint robotID,
		float timeSinceLastMsg_sec)
	{
		this.robotID = robotID;
		this.timeSinceLastMsg_sec = timeSinceLastMsg_sec;
	}

	public G2U_RobotDisconnected(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//timeSinceLastMsg_sec
		timeSinceLastMsg_sec = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		writer.Write((float)timeSinceLastMsg_sec);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//timeSinceLastMsg_sec
		timeSinceLastMsg_sec = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			//timeSinceLastMsg_sec
			result += 4; // = float_32
			return result;
		}
	}
}

public class G2U_UiDeviceConnected
{
	public uint deviceID;
	public byte successful;

	/**** Constructors ****/

	public G2U_UiDeviceConnected()
	{
	}

	public G2U_UiDeviceConnected(uint deviceID,
		byte successful)
	{
		this.deviceID = deviceID;
		this.successful = successful;
	}

	public G2U_UiDeviceConnected(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (uint)(reader.ReadUInt32());
		//successful
		successful = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)deviceID);
		writer.Write((byte)successful);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (uint)(reader.ReadUInt32());
		//successful
		successful = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//deviceID
			result += 4; // = uint_32
			//successful
			result += 1; // = uint_8
			return result;
		}
	}
}

public class G2U_RobotState
{
	public float pose_x;
	public float pose_y;
	public float pose_z;
	public float poseAngle_rad;
	public float leftWheelSpeed_mmps;
	public float rightWheelSpeed_mmps;
	public float headAngle_rad;
	public float liftHeight_mm;
	public float batteryVoltage;
	public int carryingObjectID;
	public byte status;
	public byte robotID;

	/**** Constructors ****/

	public G2U_RobotState()
	{
	}

	public G2U_RobotState(float pose_x,
		float pose_y,
		float pose_z,
		float poseAngle_rad,
		float leftWheelSpeed_mmps,
		float rightWheelSpeed_mmps,
		float headAngle_rad,
		float liftHeight_mm,
		float batteryVoltage,
		int carryingObjectID,
		byte status,
		byte robotID)
	{
		this.pose_x = pose_x;
		this.pose_y = pose_y;
		this.pose_z = pose_z;
		this.poseAngle_rad = poseAngle_rad;
		this.leftWheelSpeed_mmps = leftWheelSpeed_mmps;
		this.rightWheelSpeed_mmps = rightWheelSpeed_mmps;
		this.headAngle_rad = headAngle_rad;
		this.liftHeight_mm = liftHeight_mm;
		this.batteryVoltage = batteryVoltage;
		this.carryingObjectID = carryingObjectID;
		this.status = status;
		this.robotID = robotID;
	}

	public G2U_RobotState(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//pose_x
		pose_x = (float)(reader.ReadSingle());
		//pose_y
		pose_y = (float)(reader.ReadSingle());
		//pose_z
		pose_z = (float)(reader.ReadSingle());
		//poseAngle_rad
		poseAngle_rad = (float)(reader.ReadSingle());
		//leftWheelSpeed_mmps
		leftWheelSpeed_mmps = (float)(reader.ReadSingle());
		//rightWheelSpeed_mmps
		rightWheelSpeed_mmps = (float)(reader.ReadSingle());
		//headAngle_rad
		headAngle_rad = (float)(reader.ReadSingle());
		//liftHeight_mm
		liftHeight_mm = (float)(reader.ReadSingle());
		//batteryVoltage
		batteryVoltage = (float)(reader.ReadSingle());
		//carryingObjectID
		carryingObjectID = (int)(reader.ReadInt32());
		//status
		status = (byte)(reader.ReadByte());
		//robotID
		robotID = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)pose_x);
		writer.Write((float)pose_y);
		writer.Write((float)pose_z);
		writer.Write((float)poseAngle_rad);
		writer.Write((float)leftWheelSpeed_mmps);
		writer.Write((float)rightWheelSpeed_mmps);
		writer.Write((float)headAngle_rad);
		writer.Write((float)liftHeight_mm);
		writer.Write((float)batteryVoltage);
		writer.Write((int)carryingObjectID);
		writer.Write((byte)status);
		writer.Write((byte)robotID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//pose_x
		pose_x = (float)(reader.ReadSingle());
		//pose_y
		pose_y = (float)(reader.ReadSingle());
		//pose_z
		pose_z = (float)(reader.ReadSingle());
		//poseAngle_rad
		poseAngle_rad = (float)(reader.ReadSingle());
		//leftWheelSpeed_mmps
		leftWheelSpeed_mmps = (float)(reader.ReadSingle());
		//rightWheelSpeed_mmps
		rightWheelSpeed_mmps = (float)(reader.ReadSingle());
		//headAngle_rad
		headAngle_rad = (float)(reader.ReadSingle());
		//liftHeight_mm
		liftHeight_mm = (float)(reader.ReadSingle());
		//batteryVoltage
		batteryVoltage = (float)(reader.ReadSingle());
		//carryingObjectID
		carryingObjectID = (int)(reader.ReadInt32());
		//status
		status = (byte)(reader.ReadByte());
		//robotID
		robotID = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//pose_x
			result += 4; // = float_32
			//pose_y
			result += 4; // = float_32
			//pose_z
			result += 4; // = float_32
			//poseAngle_rad
			result += 4; // = float_32
			//leftWheelSpeed_mmps
			result += 4; // = float_32
			//rightWheelSpeed_mmps
			result += 4; // = float_32
			//headAngle_rad
			result += 4; // = float_32
			//liftHeight_mm
			result += 4; // = float_32
			//batteryVoltage
			result += 4; // = float_32
			//carryingObjectID
			result += 4; // = int_32
			//status
			result += 1; // = uint_8
			//robotID
			result += 1; // = uint_8
			return result;
		}
	}
}

public class G2U_ImageChunk
{
	public uint imageId;
	public uint frameTimeStamp;
	public ushort nrows;
	public ushort ncols;
	public ushort chunkSize;
	public byte imageEncoding;
	public byte imageChunkCount;
	public byte chunkId;
	public byte[] data;

	/**** Constructors ****/

	public G2U_ImageChunk()
	{
		this.data = new byte[1024];
	}

	public G2U_ImageChunk(uint imageId,
		uint frameTimeStamp,
		ushort nrows,
		ushort ncols,
		ushort chunkSize,
		byte imageEncoding,
		byte imageChunkCount,
		byte chunkId,
		byte[] data)
	{
		this.imageId = imageId;
		this.frameTimeStamp = frameTimeStamp;
		this.nrows = nrows;
		this.ncols = ncols;
		this.chunkSize = chunkSize;
		this.imageEncoding = imageEncoding;
		this.imageChunkCount = imageChunkCount;
		this.chunkId = chunkId;
		if (data.Length != 1024) {
			throw new System.ArgumentException("Argument has wrong length. Expected length 1024.", "data");
		}
		this.data = data;
	}

	public G2U_ImageChunk(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//imageId
		imageId = (uint)(reader.ReadUInt32());
		//frameTimeStamp
		frameTimeStamp = (uint)(reader.ReadUInt32());
		//nrows
		nrows = (ushort)(reader.ReadUInt16());
		//ncols
		ncols = (ushort)(reader.ReadUInt16());
		//chunkSize
		chunkSize = (ushort)(reader.ReadUInt16());
		//imageEncoding
		imageEncoding = (byte)(reader.ReadByte());
		//imageChunkCount
		imageChunkCount = (byte)(reader.ReadByte());
		//chunkId
		chunkId = (byte)(reader.ReadByte());
		//data
		for (int i = 0; i < 1024; i++) {
			data[i] = (byte)reader.ReadByte();
		}
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)imageId);
		writer.Write((uint)frameTimeStamp);
		writer.Write((ushort)nrows);
		writer.Write((ushort)ncols);
		writer.Write((ushort)chunkSize);
		writer.Write((byte)imageEncoding);
		writer.Write((byte)imageChunkCount);
		writer.Write((byte)chunkId);
		for (int i = 0; i < data.Length; i++) {
			writer.Write((byte)data[i]);
		}
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//imageId
		imageId = (uint)(reader.ReadUInt32());
		//frameTimeStamp
		frameTimeStamp = (uint)(reader.ReadUInt32());
		//nrows
		nrows = (ushort)(reader.ReadUInt16());
		//ncols
		ncols = (ushort)(reader.ReadUInt16());
		//chunkSize
		chunkSize = (ushort)(reader.ReadUInt16());
		//imageEncoding
		imageEncoding = (byte)(reader.ReadByte());
		//imageChunkCount
		imageChunkCount = (byte)(reader.ReadByte());
		//chunkId
		chunkId = (byte)(reader.ReadByte());
		//data
		for (int i = 0; i < 1024; i++) {
			data[i] = (byte)reader.ReadByte();
		}
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//imageId
			result += 4; // = uint_32
			//frameTimeStamp
			result += 4; // = uint_32
			//nrows
			result += 2; // = uint_16
			//ncols
			result += 2; // = uint_16
			//chunkSize
			result += 2; // = uint_16
			//imageEncoding
			result += 1; // = uint_8
			//imageChunkCount
			result += 1; // = uint_8
			//chunkId
			result += 1; // = uint_8
			//data
			result += 1 * 1024; // = uint_8 * 1024
			return result;
		}
	}
}

public class G2U_RobotObservedObject
{
	public uint robotID;
	public uint objectFamily;
	public uint objectType;
	public int objectID;
	public float img_topLeft_x;
	public float img_topLeft_y;
	public float img_width;
	public float img_height;
	public float world_x;
	public float world_y;
	public float world_z;
	public float quaternion0;
	public float quaternion1;
	public float quaternion2;
	public float quaternion3;

	/**** Constructors ****/

	public G2U_RobotObservedObject()
	{
	}

	public G2U_RobotObservedObject(uint robotID,
		uint objectFamily,
		uint objectType,
		int objectID,
		float img_topLeft_x,
		float img_topLeft_y,
		float img_width,
		float img_height,
		float world_x,
		float world_y,
		float world_z,
		float quaternion0,
		float quaternion1,
		float quaternion2,
		float quaternion3)
	{
		this.robotID = robotID;
		this.objectFamily = objectFamily;
		this.objectType = objectType;
		this.objectID = objectID;
		this.img_topLeft_x = img_topLeft_x;
		this.img_topLeft_y = img_topLeft_y;
		this.img_width = img_width;
		this.img_height = img_height;
		this.world_x = world_x;
		this.world_y = world_y;
		this.world_z = world_z;
		this.quaternion0 = quaternion0;
		this.quaternion1 = quaternion1;
		this.quaternion2 = quaternion2;
		this.quaternion3 = quaternion3;
	}

	public G2U_RobotObservedObject(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//objectFamily
		objectFamily = (uint)(reader.ReadUInt32());
		//objectType
		objectType = (uint)(reader.ReadUInt32());
		//objectID
		objectID = (int)(reader.ReadInt32());
		//img_topLeft_x
		img_topLeft_x = (float)(reader.ReadSingle());
		//img_topLeft_y
		img_topLeft_y = (float)(reader.ReadSingle());
		//img_width
		img_width = (float)(reader.ReadSingle());
		//img_height
		img_height = (float)(reader.ReadSingle());
		//world_x
		world_x = (float)(reader.ReadSingle());
		//world_y
		world_y = (float)(reader.ReadSingle());
		//world_z
		world_z = (float)(reader.ReadSingle());
		//quaternion0
		quaternion0 = (float)(reader.ReadSingle());
		//quaternion1
		quaternion1 = (float)(reader.ReadSingle());
		//quaternion2
		quaternion2 = (float)(reader.ReadSingle());
		//quaternion3
		quaternion3 = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		writer.Write((uint)objectFamily);
		writer.Write((uint)objectType);
		writer.Write((int)objectID);
		writer.Write((float)img_topLeft_x);
		writer.Write((float)img_topLeft_y);
		writer.Write((float)img_width);
		writer.Write((float)img_height);
		writer.Write((float)world_x);
		writer.Write((float)world_y);
		writer.Write((float)world_z);
		writer.Write((float)quaternion0);
		writer.Write((float)quaternion1);
		writer.Write((float)quaternion2);
		writer.Write((float)quaternion3);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//objectFamily
		objectFamily = (uint)(reader.ReadUInt32());
		//objectType
		objectType = (uint)(reader.ReadUInt32());
		//objectID
		objectID = (int)(reader.ReadInt32());
		//img_topLeft_x
		img_topLeft_x = (float)(reader.ReadSingle());
		//img_topLeft_y
		img_topLeft_y = (float)(reader.ReadSingle());
		//img_width
		img_width = (float)(reader.ReadSingle());
		//img_height
		img_height = (float)(reader.ReadSingle());
		//world_x
		world_x = (float)(reader.ReadSingle());
		//world_y
		world_y = (float)(reader.ReadSingle());
		//world_z
		world_z = (float)(reader.ReadSingle());
		//quaternion0
		quaternion0 = (float)(reader.ReadSingle());
		//quaternion1
		quaternion1 = (float)(reader.ReadSingle());
		//quaternion2
		quaternion2 = (float)(reader.ReadSingle());
		//quaternion3
		quaternion3 = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			//objectFamily
			result += 4; // = uint_32
			//objectType
			result += 4; // = uint_32
			//objectID
			result += 4; // = int_32
			//img_topLeft_x
			result += 4; // = float_32
			//img_topLeft_y
			result += 4; // = float_32
			//img_width
			result += 4; // = float_32
			//img_height
			result += 4; // = float_32
			//world_x
			result += 4; // = float_32
			//world_y
			result += 4; // = float_32
			//world_z
			result += 4; // = float_32
			//quaternion0
			result += 4; // = float_32
			//quaternion1
			result += 4; // = float_32
			//quaternion2
			result += 4; // = float_32
			//quaternion3
			result += 4; // = float_32
			return result;
		}
	}
}

public class G2U_RobotObservedNothing
{
	public uint robotID;

	/**** Constructors ****/

	public G2U_RobotObservedNothing()
	{
	}

	public G2U_RobotObservedNothing(uint robotID)
	{
		this.robotID = robotID;
	}

	public G2U_RobotObservedNothing(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			return result;
		}
	}
}

public class G2U_RobotDeletedObject
{
	public uint robotID;
	public uint objectID;

	/**** Constructors ****/

	public G2U_RobotDeletedObject()
	{
	}

	public G2U_RobotDeletedObject(uint robotID,
		uint objectID)
	{
		this.robotID = robotID;
		this.objectID = objectID;
	}

	public G2U_RobotDeletedObject(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//objectID
		objectID = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		writer.Write((uint)objectID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//objectID
		objectID = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			//objectID
			result += 4; // = uint_32
			return result;
		}
	}
}

public class G2U_DeviceDetectedVisionMarker
{
	public uint markerType;
	public float x_upperLeft;
	public float y_upperLeft;
	public float x_lowerLeft;
	public float y_lowerLeft;
	public float x_upperRight;
	public float y_upperRight;
	public float x_lowerRight;
	public float y_lowerRight;

	/**** Constructors ****/

	public G2U_DeviceDetectedVisionMarker()
	{
	}

	public G2U_DeviceDetectedVisionMarker(uint markerType,
		float x_upperLeft,
		float y_upperLeft,
		float x_lowerLeft,
		float y_lowerLeft,
		float x_upperRight,
		float y_upperRight,
		float x_lowerRight,
		float y_lowerRight)
	{
		this.markerType = markerType;
		this.x_upperLeft = x_upperLeft;
		this.y_upperLeft = y_upperLeft;
		this.x_lowerLeft = x_lowerLeft;
		this.y_lowerLeft = y_lowerLeft;
		this.x_upperRight = x_upperRight;
		this.y_upperRight = y_upperRight;
		this.x_lowerRight = x_lowerRight;
		this.y_lowerRight = y_lowerRight;
	}

	public G2U_DeviceDetectedVisionMarker(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//markerType
		markerType = (uint)(reader.ReadUInt32());
		//x_upperLeft
		x_upperLeft = (float)(reader.ReadSingle());
		//y_upperLeft
		y_upperLeft = (float)(reader.ReadSingle());
		//x_lowerLeft
		x_lowerLeft = (float)(reader.ReadSingle());
		//y_lowerLeft
		y_lowerLeft = (float)(reader.ReadSingle());
		//x_upperRight
		x_upperRight = (float)(reader.ReadSingle());
		//y_upperRight
		y_upperRight = (float)(reader.ReadSingle());
		//x_lowerRight
		x_lowerRight = (float)(reader.ReadSingle());
		//y_lowerRight
		y_lowerRight = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)markerType);
		writer.Write((float)x_upperLeft);
		writer.Write((float)y_upperLeft);
		writer.Write((float)x_lowerLeft);
		writer.Write((float)y_lowerLeft);
		writer.Write((float)x_upperRight);
		writer.Write((float)y_upperRight);
		writer.Write((float)x_lowerRight);
		writer.Write((float)y_lowerRight);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//markerType
		markerType = (uint)(reader.ReadUInt32());
		//x_upperLeft
		x_upperLeft = (float)(reader.ReadSingle());
		//y_upperLeft
		y_upperLeft = (float)(reader.ReadSingle());
		//x_lowerLeft
		x_lowerLeft = (float)(reader.ReadSingle());
		//y_lowerLeft
		y_lowerLeft = (float)(reader.ReadSingle());
		//x_upperRight
		x_upperRight = (float)(reader.ReadSingle());
		//y_upperRight
		y_upperRight = (float)(reader.ReadSingle());
		//x_lowerRight
		x_lowerRight = (float)(reader.ReadSingle());
		//y_lowerRight
		y_lowerRight = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//markerType
			result += 4; // = uint_32
			//x_upperLeft
			result += 4; // = float_32
			//y_upperLeft
			result += 4; // = float_32
			//x_lowerLeft
			result += 4; // = float_32
			//y_lowerLeft
			result += 4; // = float_32
			//x_upperRight
			result += 4; // = float_32
			//y_upperRight
			result += 4; // = float_32
			//x_lowerRight
			result += 4; // = float_32
			//y_lowerRight
			result += 4; // = float_32
			return result;
		}
	}
}

public class G2U_RobotCompletedAction
{
	public uint robotID;
	public byte success;

	/**** Constructors ****/

	public G2U_RobotCompletedAction()
	{
	}

	public G2U_RobotCompletedAction(uint robotID,
		byte success)
	{
		this.robotID = robotID;
		this.success = success;
	}

	public G2U_RobotCompletedAction(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//success
		success = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)robotID);
		writer.Write((byte)success);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (uint)(reader.ReadUInt32());
		//success
		success = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 4; // = uint_32
			//success
			result += 1; // = uint_8
			return result;
		}
	}
}

public class G2U_PlaySound
{
	public string soundFilename;
	public byte numLoops;
	public byte volume;

	/**** Constructors ****/

	public G2U_PlaySound()
	{
	}

	public G2U_PlaySound(string soundFilename,
		byte numLoops,
		byte volume)
	{
		this.soundFilename = soundFilename;
		this.numLoops = numLoops;
		this.volume = volume;
	}

	public G2U_PlaySound(System.IO.Stream stream)
		: this()
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//soundFilename
		byte soundFilename_len = reader.ReadByte();
		byte[] soundFilename_buf = reader.ReadBytes((int)soundFilename_len);
		soundFilename = System.Text.Encoding.UTF8.GetString(soundFilename_buf, 0, soundFilename_len);
		//numLoops
		numLoops = (byte)(reader.ReadByte());
		//volume
		volume = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)(soundFilename.Length));
		writer.Write(System.Text.Encoding.UTF8.GetBytes(soundFilename));
		writer.Write((byte)numLoops);
		writer.Write((byte)volume);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//soundFilename
		byte soundFilename_len = reader.ReadByte();
		byte[] soundFilename_buf = reader.ReadBytes((int)soundFilename_len);
		soundFilename = System.Text.Encoding.UTF8.GetString(soundFilename_buf, 0, soundFilename_len);
		//numLoops
		numLoops = (byte)(reader.ReadByte());
		//volume
		volume = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//soundFilename
			result += 1; //length = uint_8
			result += 1 * soundFilename.Length; //string
			//numLoops
			result += 1; // = uint_8
			//volume
			result += 1; // = uint_8
			return result;
		}
	}
}

public class G2U_StopSound
{

	/**** Constructors ****/

	public G2U_StopSound()
	{
	}

	public G2U_StopSound(System.IO.Stream stream)
		: this()
	{
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			return result;
		}
	}
}

public class G2U_Message {
	public enum Tag {
		Ping,	//0
		RobotAvailable,	//1
		UiDeviceAvailable,	//2
		RobotConnected,	//3
		RobotDisconnected,	//4
		UiDeviceConnected,	//5
		RobotState,	//6
		ImageChunk,	//7
		RobotObservedObject,	//8
		RobotObservedNothing,	//9
		RobotDeletedObject,	//10
		DeviceDetectedVisionMarker,	//11
		RobotCompletedAction,	//12
		PlaySound,	//13
		StopSound,	//14
		INVALID
	};

	private Tag _tag = Tag.INVALID;

	public Tag GetTag() { return _tag; }

	private object _state = null;

	public G2U_Ping Ping
	{
		get {
			if (_tag != Tag.Ping) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"Ping\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_Ping)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.Ping : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotAvailable RobotAvailable
	{
		get {
			if (_tag != Tag.RobotAvailable) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotAvailable\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotAvailable)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotAvailable : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_UiDeviceAvailable UiDeviceAvailable
	{
		get {
			if (_tag != Tag.UiDeviceAvailable) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"UiDeviceAvailable\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_UiDeviceAvailable)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.UiDeviceAvailable : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotConnected RobotConnected
	{
		get {
			if (_tag != Tag.RobotConnected) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotConnected\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotConnected)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotConnected : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotDisconnected RobotDisconnected
	{
		get {
			if (_tag != Tag.RobotDisconnected) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotDisconnected\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotDisconnected)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotDisconnected : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_UiDeviceConnected UiDeviceConnected
	{
		get {
			if (_tag != Tag.UiDeviceConnected) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"UiDeviceConnected\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_UiDeviceConnected)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.UiDeviceConnected : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotState RobotState
	{
		get {
			if (_tag != Tag.RobotState) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotState\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotState)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotState : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_ImageChunk ImageChunk
	{
		get {
			if (_tag != Tag.ImageChunk) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ImageChunk\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_ImageChunk)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ImageChunk : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotObservedObject RobotObservedObject
	{
		get {
			if (_tag != Tag.RobotObservedObject) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotObservedObject\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotObservedObject)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotObservedObject : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotObservedNothing RobotObservedNothing
	{
		get {
			if (_tag != Tag.RobotObservedNothing) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotObservedNothing\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotObservedNothing)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotObservedNothing : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotDeletedObject RobotDeletedObject
	{
		get {
			if (_tag != Tag.RobotDeletedObject) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotDeletedObject\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotDeletedObject)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotDeletedObject : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_DeviceDetectedVisionMarker DeviceDetectedVisionMarker
	{
		get {
			if (_tag != Tag.DeviceDetectedVisionMarker) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"DeviceDetectedVisionMarker\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_DeviceDetectedVisionMarker)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.DeviceDetectedVisionMarker : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_RobotCompletedAction RobotCompletedAction
	{
		get {
			if (_tag != Tag.RobotCompletedAction) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"RobotCompletedAction\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_RobotCompletedAction)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.RobotCompletedAction : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_PlaySound PlaySound
	{
		get {
			if (_tag != Tag.PlaySound) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"PlaySound\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_PlaySound)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.PlaySound : Tag.INVALID;
			_state = value;
		}
	}

	public G2U_StopSound StopSound
	{
		get {
			if (_tag != Tag.StopSound) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StopSound\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (G2U_StopSound)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StopSound : Tag.INVALID;
			_state = value;
		}
	}

	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		Tag newTag = Tag.INVALID;
		newTag = (Tag)reader.ReadByte();
		switch(newTag) {
		case Tag.Ping:
			_state = new G2U_Ping(stream);
			break;
		case Tag.RobotAvailable:
			_state = new G2U_RobotAvailable(stream);
			break;
		case Tag.UiDeviceAvailable:
			_state = new G2U_UiDeviceAvailable(stream);
			break;
		case Tag.RobotConnected:
			_state = new G2U_RobotConnected(stream);
			break;
		case Tag.RobotDisconnected:
			_state = new G2U_RobotDisconnected(stream);
			break;
		case Tag.UiDeviceConnected:
			_state = new G2U_UiDeviceConnected(stream);
			break;
		case Tag.RobotState:
			_state = new G2U_RobotState(stream);
			break;
		case Tag.ImageChunk:
			_state = new G2U_ImageChunk(stream);
			break;
		case Tag.RobotObservedObject:
			_state = new G2U_RobotObservedObject(stream);
			break;
		case Tag.RobotObservedNothing:
			_state = new G2U_RobotObservedNothing(stream);
			break;
		case Tag.RobotDeletedObject:
			_state = new G2U_RobotDeletedObject(stream);
			break;
		case Tag.DeviceDetectedVisionMarker:
			_state = new G2U_DeviceDetectedVisionMarker(stream);
			break;
		case Tag.RobotCompletedAction:
			_state = new G2U_RobotCompletedAction(stream);
			break;
		case Tag.PlaySound:
			_state = new G2U_PlaySound(stream);
			break;
		case Tag.StopSound:
			_state = new G2U_StopSound(stream);
			break;
		default:
			break;
		}
		_tag = newTag;
		return stream;
	}

	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)GetTag());
		switch(GetTag()) {
		case Tag.Ping:
			Ping.Pack(stream);
			break;
		case Tag.RobotAvailable:
			RobotAvailable.Pack(stream);
			break;
		case Tag.UiDeviceAvailable:
			UiDeviceAvailable.Pack(stream);
			break;
		case Tag.RobotConnected:
			RobotConnected.Pack(stream);
			break;
		case Tag.RobotDisconnected:
			RobotDisconnected.Pack(stream);
			break;
		case Tag.UiDeviceConnected:
			UiDeviceConnected.Pack(stream);
			break;
		case Tag.RobotState:
			RobotState.Pack(stream);
			break;
		case Tag.ImageChunk:
			ImageChunk.Pack(stream);
			break;
		case Tag.RobotObservedObject:
			RobotObservedObject.Pack(stream);
			break;
		case Tag.RobotObservedNothing:
			RobotObservedNothing.Pack(stream);
			break;
		case Tag.RobotDeletedObject:
			RobotDeletedObject.Pack(stream);
			break;
		case Tag.DeviceDetectedVisionMarker:
			DeviceDetectedVisionMarker.Pack(stream);
			break;
		case Tag.RobotCompletedAction:
			RobotCompletedAction.Pack(stream);
			break;
		case Tag.PlaySound:
			PlaySound.Pack(stream);
			break;
		case Tag.StopSound:
			StopSound.Pack(stream);
			break;
		default:
			break;
		}
		return stream;
	}

	public int Size
	{
		get {
			int result = 1; // tag = uint_8
			switch(GetTag()) {
			case Tag.Ping:
				result += Ping.Size;
				break;
			case Tag.RobotAvailable:
				result += RobotAvailable.Size;
				break;
			case Tag.UiDeviceAvailable:
				result += UiDeviceAvailable.Size;
				break;
			case Tag.RobotConnected:
				result += RobotConnected.Size;
				break;
			case Tag.RobotDisconnected:
				result += RobotDisconnected.Size;
				break;
			case Tag.UiDeviceConnected:
				result += UiDeviceConnected.Size;
				break;
			case Tag.RobotState:
				result += RobotState.Size;
				break;
			case Tag.ImageChunk:
				result += ImageChunk.Size;
				break;
			case Tag.RobotObservedObject:
				result += RobotObservedObject.Size;
				break;
			case Tag.RobotObservedNothing:
				result += RobotObservedNothing.Size;
				break;
			case Tag.RobotDeletedObject:
				result += RobotDeletedObject.Size;
				break;
			case Tag.DeviceDetectedVisionMarker:
				result += DeviceDetectedVisionMarker.Size;
				break;
			case Tag.RobotCompletedAction:
				result += RobotCompletedAction.Size;
				break;
			case Tag.PlaySound:
				result += PlaySound.Size;
				break;
			case Tag.StopSound:
				result += StopSound.Size;
				break;
			default:
				return 0;
			}
			return result;
		}
	}
}

}//namespace Anki.Cozmo
