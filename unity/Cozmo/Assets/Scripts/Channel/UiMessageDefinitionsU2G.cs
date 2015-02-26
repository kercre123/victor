namespace Anki.Cozmo
{
// Generated from /Users/gregnage/cozmo-game/src/UiMessageDefinitionsU2G.clad

public class U2G_Ping
{
	public uint counter;

	/**** Constructors ****/

	public U2G_Ping()
	{
	}

	public U2G_Ping(uint counter)
	{
		this.counter = counter;
	}

	public U2G_Ping(System.IO.Stream stream)
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

public class U2G_ConnectToRobot
{
	public byte robotID;

	/**** Constructors ****/

	public U2G_ConnectToRobot()
	{
	}

	public U2G_ConnectToRobot(byte robotID)
	{
		this.robotID = robotID;
	}

	public U2G_ConnectToRobot(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)robotID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_ConnectToUiDevice
{
	public byte deviceID;

	/**** Constructors ****/

	public U2G_ConnectToUiDevice()
	{
	}

	public U2G_ConnectToUiDevice(byte deviceID)
	{
		this.deviceID = deviceID;
	}

	public U2G_ConnectToUiDevice(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)deviceID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//deviceID
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_DisconnectFromUiDevice
{
	public byte deviceID;

	/**** Constructors ****/

	public U2G_DisconnectFromUiDevice()
	{
	}

	public U2G_DisconnectFromUiDevice(byte deviceID)
	{
		this.deviceID = deviceID;
	}

	public U2G_DisconnectFromUiDevice(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)deviceID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//deviceID
		deviceID = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//deviceID
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_ForceAddRobot
{
	public byte[] ipAddress;
	public byte robotID;
	public byte isSimulated;

	/**** Constructors ****/

	public U2G_ForceAddRobot()
	{
		this.ipAddress = new byte[16];
	}

	public U2G_ForceAddRobot(byte[] ipAddress,
		byte robotID,
		byte isSimulated)
	{
		if (ipAddress.Length != 16) {
			throw new System.ArgumentException("Argument has wrong length. Expected length 16.", "ipAddress");
		}
		this.ipAddress = ipAddress;
		this.robotID = robotID;
		this.isSimulated = isSimulated;
	}

	public U2G_ForceAddRobot(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//ipAddress
		for (int i = 0; i < 16; i++) {
			ipAddress[i] = (byte)reader.ReadByte();
		}
		//robotID
		robotID = (byte)(reader.ReadByte());
		//isSimulated
		isSimulated = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		for (int i = 0; i < ipAddress.Length; i++) {
			writer.Write((byte)ipAddress[i]);
		}
		writer.Write((byte)robotID);
		writer.Write((byte)isSimulated);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//ipAddress
		for (int i = 0; i < 16; i++) {
			ipAddress[i] = (byte)reader.ReadByte();
		}
		//robotID
		robotID = (byte)(reader.ReadByte());
		//isSimulated
		isSimulated = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//ipAddress
			result += 1 * 16; // = uint_8 * 16
			//robotID
			result += 1; // = uint_8
			//isSimulated
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_StartEngine
{
	public byte asHost;
	public byte[] vizHostIP;

	/**** Constructors ****/

	public U2G_StartEngine()
	{
		this.vizHostIP = new byte[16];
	}

	public U2G_StartEngine(byte asHost,
		byte[] vizHostIP)
	{
		this.asHost = asHost;
		if (vizHostIP.Length != 16) {
			throw new System.ArgumentException("Argument has wrong length. Expected length 16.", "vizHostIP");
		}
		this.vizHostIP = vizHostIP;
	}

	public U2G_StartEngine(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//asHost
		asHost = (byte)(reader.ReadByte());
		//vizHostIP
		for (int i = 0; i < 16; i++) {
			vizHostIP[i] = (byte)reader.ReadByte();
		}
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)asHost);
		for (int i = 0; i < vizHostIP.Length; i++) {
			writer.Write((byte)vizHostIP[i]);
		}
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//asHost
		asHost = (byte)(reader.ReadByte());
		//vizHostIP
		for (int i = 0; i < 16; i++) {
			vizHostIP[i] = (byte)reader.ReadByte();
		}
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//asHost
			result += 1; // = uint_8
			//vizHostIP
			result += 1 * 16; // = uint_8 * 16
			return result;
		}
	}
}

public class U2G_DriveWheels
{
	public float lwheel_speed_mmps;
	public float rwheel_speed_mmps;

	/**** Constructors ****/

	public U2G_DriveWheels()
	{
	}

	public U2G_DriveWheels(float lwheel_speed_mmps,
		float rwheel_speed_mmps)
	{
		this.lwheel_speed_mmps = lwheel_speed_mmps;
		this.rwheel_speed_mmps = rwheel_speed_mmps;
	}

	public U2G_DriveWheels(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//lwheel_speed_mmps
		lwheel_speed_mmps = (float)(reader.ReadSingle());
		//rwheel_speed_mmps
		rwheel_speed_mmps = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)lwheel_speed_mmps);
		writer.Write((float)rwheel_speed_mmps);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//lwheel_speed_mmps
		lwheel_speed_mmps = (float)(reader.ReadSingle());
		//rwheel_speed_mmps
		rwheel_speed_mmps = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//lwheel_speed_mmps
			result += 4; // = float_32
			//rwheel_speed_mmps
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_TurnInPlace
{
	public float angle_rad;
	public byte robotID;

	/**** Constructors ****/

	public U2G_TurnInPlace()
	{
	}

	public U2G_TurnInPlace(float angle_rad,
		byte robotID)
	{
		this.angle_rad = angle_rad;
		this.robotID = robotID;
	}

	public U2G_TurnInPlace(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//angle_rad
		angle_rad = (float)(reader.ReadSingle());
		//robotID
		robotID = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)angle_rad);
		writer.Write((byte)robotID);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//angle_rad
		angle_rad = (float)(reader.ReadSingle());
		//robotID
		robotID = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//angle_rad
			result += 4; // = float_32
			//robotID
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_MoveHead
{
	public float speed_rad_per_sec;

	/**** Constructors ****/

	public U2G_MoveHead()
	{
	}

	public U2G_MoveHead(float speed_rad_per_sec)
	{
		this.speed_rad_per_sec = speed_rad_per_sec;
	}

	public U2G_MoveHead(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//speed_rad_per_sec
		speed_rad_per_sec = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)speed_rad_per_sec);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//speed_rad_per_sec
		speed_rad_per_sec = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//speed_rad_per_sec
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_MoveLift
{
	public float speed_rad_per_sec;

	/**** Constructors ****/

	public U2G_MoveLift()
	{
	}

	public U2G_MoveLift(float speed_rad_per_sec)
	{
		this.speed_rad_per_sec = speed_rad_per_sec;
	}

	public U2G_MoveLift(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//speed_rad_per_sec
		speed_rad_per_sec = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)speed_rad_per_sec);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//speed_rad_per_sec
		speed_rad_per_sec = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//speed_rad_per_sec
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_SetLiftHeight
{
	public float height_mm;
	public float max_speed_rad_per_sec;
	public float accel_rad_per_sec2;

	/**** Constructors ****/

	public U2G_SetLiftHeight()
	{
	}

	public U2G_SetLiftHeight(float height_mm,
		float max_speed_rad_per_sec,
		float accel_rad_per_sec2)
	{
		this.height_mm = height_mm;
		this.max_speed_rad_per_sec = max_speed_rad_per_sec;
		this.accel_rad_per_sec2 = accel_rad_per_sec2;
	}

	public U2G_SetLiftHeight(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//height_mm
		height_mm = (float)(reader.ReadSingle());
		//max_speed_rad_per_sec
		max_speed_rad_per_sec = (float)(reader.ReadSingle());
		//accel_rad_per_sec2
		accel_rad_per_sec2 = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)height_mm);
		writer.Write((float)max_speed_rad_per_sec);
		writer.Write((float)accel_rad_per_sec2);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//height_mm
		height_mm = (float)(reader.ReadSingle());
		//max_speed_rad_per_sec
		max_speed_rad_per_sec = (float)(reader.ReadSingle());
		//accel_rad_per_sec2
		accel_rad_per_sec2 = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//height_mm
			result += 4; // = float_32
			//max_speed_rad_per_sec
			result += 4; // = float_32
			//accel_rad_per_sec2
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_SetHeadAngle
{
	public float angle_rad;
	public float max_speed_rad_per_sec;
	public float accel_rad_per_sec2;

	/**** Constructors ****/

	public U2G_SetHeadAngle()
	{
	}

	public U2G_SetHeadAngle(float angle_rad,
		float max_speed_rad_per_sec,
		float accel_rad_per_sec2)
	{
		this.angle_rad = angle_rad;
		this.max_speed_rad_per_sec = max_speed_rad_per_sec;
		this.accel_rad_per_sec2 = accel_rad_per_sec2;
	}

	public U2G_SetHeadAngle(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//angle_rad
		angle_rad = (float)(reader.ReadSingle());
		//max_speed_rad_per_sec
		max_speed_rad_per_sec = (float)(reader.ReadSingle());
		//accel_rad_per_sec2
		accel_rad_per_sec2 = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)angle_rad);
		writer.Write((float)max_speed_rad_per_sec);
		writer.Write((float)accel_rad_per_sec2);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//angle_rad
		angle_rad = (float)(reader.ReadSingle());
		//max_speed_rad_per_sec
		max_speed_rad_per_sec = (float)(reader.ReadSingle());
		//accel_rad_per_sec2
		accel_rad_per_sec2 = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//angle_rad
			result += 4; // = float_32
			//max_speed_rad_per_sec
			result += 4; // = float_32
			//accel_rad_per_sec2
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_StopAllMotors
{

	/**** Constructors ****/

	public U2G_StopAllMotors()
	{
	}

	public U2G_StopAllMotors(System.IO.Stream stream)
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

public class U2G_ImageRequest
{
	public byte robotID;
	public byte mode;

	/**** Constructors ****/

	public U2G_ImageRequest()
	{
	}

	public U2G_ImageRequest(byte robotID,
		byte mode)
	{
		this.robotID = robotID;
		this.mode = mode;
	}

	public U2G_ImageRequest(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (byte)(reader.ReadByte());
		//mode
		mode = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)robotID);
		writer.Write((byte)mode);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//robotID
		robotID = (byte)(reader.ReadByte());
		//mode
		mode = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//robotID
			result += 1; // = uint_8
			//mode
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_SetRobotImageSendMode
{
	public byte mode;
	public byte resolution;

	/**** Constructors ****/

	public U2G_SetRobotImageSendMode()
	{
	}

	public U2G_SetRobotImageSendMode(byte mode,
		byte resolution)
	{
		this.mode = mode;
		this.resolution = resolution;
	}

	public U2G_SetRobotImageSendMode(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//mode
		mode = (byte)(reader.ReadByte());
		//resolution
		resolution = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)mode);
		writer.Write((byte)resolution);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//mode
		mode = (byte)(reader.ReadByte());
		//resolution
		resolution = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//mode
			result += 1; // = uint_8
			//resolution
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_SaveImages
{
	public byte enableSave;

	/**** Constructors ****/

	public U2G_SaveImages()
	{
	}

	public U2G_SaveImages(byte enableSave)
	{
		this.enableSave = enableSave;
	}

	public U2G_SaveImages(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//enableSave
		enableSave = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)enableSave);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//enableSave
		enableSave = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//enableSave
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_EnableDisplay
{
	public byte enable;

	/**** Constructors ****/

	public U2G_EnableDisplay()
	{
	}

	public U2G_EnableDisplay(byte enable)
	{
		this.enable = enable;
	}

	public U2G_EnableDisplay(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//enable
		enable = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)enable);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//enable
		enable = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//enable
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_SetHeadlights
{
	public byte intensity;

	/**** Constructors ****/

	public U2G_SetHeadlights()
	{
	}

	public U2G_SetHeadlights(byte intensity)
	{
		this.intensity = intensity;
	}

	public U2G_SetHeadlights(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//intensity
		intensity = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)intensity);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//intensity
		intensity = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//intensity
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_GotoPose
{
	public float x_mm;
	public float y_mm;
	public float rad;
	public byte level;
	public byte useManualSpeed;

	/**** Constructors ****/

	public U2G_GotoPose()
	{
	}

	public U2G_GotoPose(float x_mm,
		float y_mm,
		float rad,
		byte level,
		byte useManualSpeed)
	{
		this.x_mm = x_mm;
		this.y_mm = y_mm;
		this.rad = rad;
		this.level = level;
		this.useManualSpeed = useManualSpeed;
	}

	public U2G_GotoPose(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)x_mm);
		writer.Write((float)y_mm);
		writer.Write((float)rad);
		writer.Write((byte)level);
		writer.Write((byte)useManualSpeed);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//x_mm
			result += 4; // = float_32
			//y_mm
			result += 4; // = float_32
			//rad
			result += 4; // = float_32
			//level
			result += 1; // = uint_8
			//useManualSpeed
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_PlaceObjectOnGround
{
	public float x_mm;
	public float y_mm;
	public float rad;
	public byte level;
	public byte useManualSpeed;

	/**** Constructors ****/

	public U2G_PlaceObjectOnGround()
	{
	}

	public U2G_PlaceObjectOnGround(float x_mm,
		float y_mm,
		float rad,
		byte level,
		byte useManualSpeed)
	{
		this.x_mm = x_mm;
		this.y_mm = y_mm;
		this.rad = rad;
		this.level = level;
		this.useManualSpeed = useManualSpeed;
	}

	public U2G_PlaceObjectOnGround(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)x_mm);
		writer.Write((float)y_mm);
		writer.Write((float)rad);
		writer.Write((byte)level);
		writer.Write((byte)useManualSpeed);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//x_mm
			result += 4; // = float_32
			//y_mm
			result += 4; // = float_32
			//rad
			result += 4; // = float_32
			//level
			result += 1; // = uint_8
			//useManualSpeed
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_PlaceObjectOnGroundHere
{

	/**** Constructors ****/

	public U2G_PlaceObjectOnGroundHere()
	{
	}

	public U2G_PlaceObjectOnGroundHere(System.IO.Stream stream)
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

public class U2G_ExecuteTestPlan
{

	/**** Constructors ****/

	public U2G_ExecuteTestPlan()
	{
	}

	public U2G_ExecuteTestPlan(System.IO.Stream stream)
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

public class U2G_SelectNextObject
{

	/**** Constructors ****/

	public U2G_SelectNextObject()
	{
	}

	public U2G_SelectNextObject(System.IO.Stream stream)
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

public class U2G_PickAndPlaceObject
{
	public int objectID;
	public byte usePreDockPose;
	public byte useManualSpeed;

	/**** Constructors ****/

	public U2G_PickAndPlaceObject()
	{
	}

	public U2G_PickAndPlaceObject(int objectID,
		byte usePreDockPose,
		byte useManualSpeed)
	{
		this.objectID = objectID;
		this.usePreDockPose = usePreDockPose;
		this.useManualSpeed = useManualSpeed;
	}

	public U2G_PickAndPlaceObject(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//objectID
		objectID = (int)(reader.ReadInt32());
		//usePreDockPose
		usePreDockPose = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((int)objectID);
		writer.Write((byte)usePreDockPose);
		writer.Write((byte)useManualSpeed);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//objectID
		objectID = (int)(reader.ReadInt32());
		//usePreDockPose
		usePreDockPose = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//objectID
			result += 4; // = int_32
			//usePreDockPose
			result += 1; // = uint_8
			//useManualSpeed
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_TraverseObject
{
	public byte usePreDockPose;
	public byte useManualSpeed;

	/**** Constructors ****/

	public U2G_TraverseObject()
	{
	}

	public U2G_TraverseObject(byte usePreDockPose,
		byte useManualSpeed)
	{
		this.usePreDockPose = usePreDockPose;
		this.useManualSpeed = useManualSpeed;
	}

	public U2G_TraverseObject(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//usePreDockPose
		usePreDockPose = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)usePreDockPose);
		writer.Write((byte)useManualSpeed);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//usePreDockPose
		usePreDockPose = (byte)(reader.ReadByte());
		//useManualSpeed
		useManualSpeed = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//usePreDockPose
			result += 1; // = uint_8
			//useManualSpeed
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_ClearAllBlocks
{

	/**** Constructors ****/

	public U2G_ClearAllBlocks()
	{
	}

	public U2G_ClearAllBlocks(System.IO.Stream stream)
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

public class U2G_ExecuteBehavior
{
	public byte behaviorMode;

	/**** Constructors ****/

	public U2G_ExecuteBehavior()
	{
	}

	public U2G_ExecuteBehavior(byte behaviorMode)
	{
		this.behaviorMode = behaviorMode;
	}

	public U2G_ExecuteBehavior(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//behaviorMode
		behaviorMode = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)behaviorMode);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//behaviorMode
		behaviorMode = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//behaviorMode
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_SetBehaviorState
{
	public byte behaviorState;

	/**** Constructors ****/

	public U2G_SetBehaviorState()
	{
	}

	public U2G_SetBehaviorState(byte behaviorState)
	{
		this.behaviorState = behaviorState;
	}

	public U2G_SetBehaviorState(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//behaviorState
		behaviorState = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)behaviorState);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//behaviorState
		behaviorState = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//behaviorState
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_AbortPath
{

	/**** Constructors ****/

	public U2G_AbortPath()
	{
	}

	public U2G_AbortPath(System.IO.Stream stream)
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

public class U2G_AbortAll
{

	/**** Constructors ****/

	public U2G_AbortAll()
	{
	}

	public U2G_AbortAll(System.IO.Stream stream)
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

public class U2G_DrawPoseMarker
{
	public float x_mm;
	public float y_mm;
	public float rad;
	public byte level;

	/**** Constructors ****/

	public U2G_DrawPoseMarker()
	{
	}

	public U2G_DrawPoseMarker(float x_mm,
		float y_mm,
		float rad,
		byte level)
	{
		this.x_mm = x_mm;
		this.y_mm = y_mm;
		this.rad = rad;
		this.level = level;
	}

	public U2G_DrawPoseMarker(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)x_mm);
		writer.Write((float)y_mm);
		writer.Write((float)rad);
		writer.Write((byte)level);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//x_mm
		x_mm = (float)(reader.ReadSingle());
		//y_mm
		y_mm = (float)(reader.ReadSingle());
		//rad
		rad = (float)(reader.ReadSingle());
		//level
		level = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//x_mm
			result += 4; // = float_32
			//y_mm
			result += 4; // = float_32
			//rad
			result += 4; // = float_32
			//level
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_ErasePoseMarker
{

	/**** Constructors ****/

	public U2G_ErasePoseMarker()
	{
	}

	public U2G_ErasePoseMarker(System.IO.Stream stream)
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

public class U2G_SetHeadControllerGains
{
	public float kp;
	public float ki;
	public float maxIntegralError;

	/**** Constructors ****/

	public U2G_SetHeadControllerGains()
	{
	}

	public U2G_SetHeadControllerGains(float kp,
		float ki,
		float maxIntegralError)
	{
		this.kp = kp;
		this.ki = ki;
		this.maxIntegralError = maxIntegralError;
	}

	public U2G_SetHeadControllerGains(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//kp
		kp = (float)(reader.ReadSingle());
		//ki
		ki = (float)(reader.ReadSingle());
		//maxIntegralError
		maxIntegralError = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)kp);
		writer.Write((float)ki);
		writer.Write((float)maxIntegralError);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//kp
		kp = (float)(reader.ReadSingle());
		//ki
		ki = (float)(reader.ReadSingle());
		//maxIntegralError
		maxIntegralError = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//kp
			result += 4; // = float_32
			//ki
			result += 4; // = float_32
			//maxIntegralError
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_SetLiftControllerGains
{
	public float kp;
	public float ki;
	public float maxIntegralError;

	/**** Constructors ****/

	public U2G_SetLiftControllerGains()
	{
	}

	public U2G_SetLiftControllerGains(float kp,
		float ki,
		float maxIntegralError)
	{
		this.kp = kp;
		this.ki = ki;
		this.maxIntegralError = maxIntegralError;
	}

	public U2G_SetLiftControllerGains(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//kp
		kp = (float)(reader.ReadSingle());
		//ki
		ki = (float)(reader.ReadSingle());
		//maxIntegralError
		maxIntegralError = (float)(reader.ReadSingle());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)kp);
		writer.Write((float)ki);
		writer.Write((float)maxIntegralError);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//kp
		kp = (float)(reader.ReadSingle());
		//ki
		ki = (float)(reader.ReadSingle());
		//maxIntegralError
		maxIntegralError = (float)(reader.ReadSingle());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//kp
			result += 4; // = float_32
			//ki
			result += 4; // = float_32
			//maxIntegralError
			result += 4; // = float_32
			return result;
		}
	}
}

public class U2G_SelectNextSoundScheme
{

	/**** Constructors ****/

	public U2G_SelectNextSoundScheme()
	{
	}

	public U2G_SelectNextSoundScheme(System.IO.Stream stream)
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

public class U2G_StartTestMode
{
	public int p1;
	public int p2;
	public int p3;
	public byte mode;

	/**** Constructors ****/

	public U2G_StartTestMode()
	{
	}

	public U2G_StartTestMode(int p1,
		int p2,
		int p3,
		byte mode)
	{
		this.p1 = p1;
		this.p2 = p2;
		this.p3 = p3;
		this.mode = mode;
	}

	public U2G_StartTestMode(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//p1
		p1 = (int)(reader.ReadInt32());
		//p2
		p2 = (int)(reader.ReadInt32());
		//p3
		p3 = (int)(reader.ReadInt32());
		//mode
		mode = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((int)p1);
		writer.Write((int)p2);
		writer.Write((int)p3);
		writer.Write((byte)mode);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//p1
		p1 = (int)(reader.ReadInt32());
		//p2
		p2 = (int)(reader.ReadInt32());
		//p3
		p3 = (int)(reader.ReadInt32());
		//mode
		mode = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//p1
			result += 4; // = int_32
			//p2
			result += 4; // = int_32
			//p3
			result += 4; // = int_32
			//mode
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_IMURequest
{
	public uint length_ms;

	/**** Constructors ****/

	public U2G_IMURequest()
	{
	}

	public U2G_IMURequest(uint length_ms)
	{
		this.length_ms = length_ms;
	}

	public U2G_IMURequest(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//length_ms
		length_ms = (uint)(reader.ReadUInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)length_ms);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//length_ms
		length_ms = (uint)(reader.ReadUInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//length_ms
			result += 4; // = uint_32
			return result;
		}
	}
}

public class U2G_PlayAnimation
{
	public uint numLoops;
	public string animationName;

	/**** Constructors ****/

	public U2G_PlayAnimation()
	{
	}

	public U2G_PlayAnimation(uint numLoops,
		string animationName)
	{
		this.numLoops = numLoops;
		this.animationName = animationName;
	}

	public U2G_PlayAnimation(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//numLoops
		numLoops = (uint)(reader.ReadUInt32());
		//animationName
		byte animationName_len = reader.ReadByte();
		byte[] animationName_buf = reader.ReadBytes((int)animationName_len);
		animationName = System.Text.Encoding.UTF8.GetString(animationName_buf, 0, animationName_len);
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((uint)numLoops);
		writer.Write((byte)(animationName.Length));
		writer.Write(System.Text.Encoding.UTF8.GetBytes(animationName));
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//numLoops
		numLoops = (uint)(reader.ReadUInt32());
		//animationName
		byte animationName_len = reader.ReadByte();
		byte[] animationName_buf = reader.ReadBytes((int)animationName_len);
		animationName = System.Text.Encoding.UTF8.GetString(animationName_buf, 0, animationName_len);
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//numLoops
			result += 4; // = uint_32
			//animationName
			result += 1; //length = uint_8
			result += 1 * animationName.Length; //string
			return result;
		}
	}
}

public class U2G_ReadAnimationFile
{

	/**** Constructors ****/

	public U2G_ReadAnimationFile()
	{
	}

	public U2G_ReadAnimationFile(System.IO.Stream stream)
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

public class U2G_StartFaceTracking
{
	public byte timeout_sec;

	/**** Constructors ****/

	public U2G_StartFaceTracking()
	{
	}

	public U2G_StartFaceTracking(byte timeout_sec)
	{
		this.timeout_sec = timeout_sec;
	}

	public U2G_StartFaceTracking(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//timeout_sec
		timeout_sec = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((byte)timeout_sec);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//timeout_sec
		timeout_sec = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//timeout_sec
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_StopFaceTracking
{

	/**** Constructors ****/

	public U2G_StopFaceTracking()
	{
	}

	public U2G_StopFaceTracking(System.IO.Stream stream)
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

public class U2G_StartLookingForMarkers
{

	/**** Constructors ****/

	public U2G_StartLookingForMarkers()
	{
	}

	public U2G_StartLookingForMarkers(System.IO.Stream stream)
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

public class U2G_StopLookingForMarkers
{

	/**** Constructors ****/

	public U2G_StopLookingForMarkers()
	{
	}

	public U2G_StopLookingForMarkers(System.IO.Stream stream)
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

public class U2G_SetVisionSystemParams
{
	public int autoexposureOn;
	public float exposureTime;
	public int integerCountsIncrement;
	public float minExposureTime;
	public float maxExposureTime;
	public float percentileToMakeHigh;
	public int limitFramerate;
	public byte highValue;

	/**** Constructors ****/

	public U2G_SetVisionSystemParams()
	{
	}

	public U2G_SetVisionSystemParams(int autoexposureOn,
		float exposureTime,
		int integerCountsIncrement,
		float minExposureTime,
		float maxExposureTime,
		float percentileToMakeHigh,
		int limitFramerate,
		byte highValue)
	{
		this.autoexposureOn = autoexposureOn;
		this.exposureTime = exposureTime;
		this.integerCountsIncrement = integerCountsIncrement;
		this.minExposureTime = minExposureTime;
		this.maxExposureTime = maxExposureTime;
		this.percentileToMakeHigh = percentileToMakeHigh;
		this.limitFramerate = limitFramerate;
		this.highValue = highValue;
	}

	public U2G_SetVisionSystemParams(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//autoexposureOn
		autoexposureOn = (int)(reader.ReadInt32());
		//exposureTime
		exposureTime = (float)(reader.ReadSingle());
		//integerCountsIncrement
		integerCountsIncrement = (int)(reader.ReadInt32());
		//minExposureTime
		minExposureTime = (float)(reader.ReadSingle());
		//maxExposureTime
		maxExposureTime = (float)(reader.ReadSingle());
		//percentileToMakeHigh
		percentileToMakeHigh = (float)(reader.ReadSingle());
		//limitFramerate
		limitFramerate = (int)(reader.ReadInt32());
		//highValue
		highValue = (byte)(reader.ReadByte());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((int)autoexposureOn);
		writer.Write((float)exposureTime);
		writer.Write((int)integerCountsIncrement);
		writer.Write((float)minExposureTime);
		writer.Write((float)maxExposureTime);
		writer.Write((float)percentileToMakeHigh);
		writer.Write((int)limitFramerate);
		writer.Write((byte)highValue);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//autoexposureOn
		autoexposureOn = (int)(reader.ReadInt32());
		//exposureTime
		exposureTime = (float)(reader.ReadSingle());
		//integerCountsIncrement
		integerCountsIncrement = (int)(reader.ReadInt32());
		//minExposureTime
		minExposureTime = (float)(reader.ReadSingle());
		//maxExposureTime
		maxExposureTime = (float)(reader.ReadSingle());
		//percentileToMakeHigh
		percentileToMakeHigh = (float)(reader.ReadSingle());
		//limitFramerate
		limitFramerate = (int)(reader.ReadInt32());
		//highValue
		highValue = (byte)(reader.ReadByte());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//autoexposureOn
			result += 4; // = int_32
			//exposureTime
			result += 4; // = float_32
			//integerCountsIncrement
			result += 4; // = int_32
			//minExposureTime
			result += 4; // = float_32
			//maxExposureTime
			result += 4; // = float_32
			//percentileToMakeHigh
			result += 4; // = float_32
			//limitFramerate
			result += 4; // = int_32
			//highValue
			result += 1; // = uint_8
			return result;
		}
	}
}

public class U2G_SetFaceDetectParams
{
	public float scaleFactor;
	public int minNeighbors;
	public int minObjectHeight;
	public int minObjectWidth;
	public int maxObjectHeight;
	public int maxObjectWidth;

	/**** Constructors ****/

	public U2G_SetFaceDetectParams()
	{
	}

	public U2G_SetFaceDetectParams(float scaleFactor,
		int minNeighbors,
		int minObjectHeight,
		int minObjectWidth,
		int maxObjectHeight,
		int maxObjectWidth)
	{
		this.scaleFactor = scaleFactor;
		this.minNeighbors = minNeighbors;
		this.minObjectHeight = minObjectHeight;
		this.minObjectWidth = minObjectWidth;
		this.maxObjectHeight = maxObjectHeight;
		this.maxObjectWidth = maxObjectWidth;
	}

	public U2G_SetFaceDetectParams(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//scaleFactor
		scaleFactor = (float)(reader.ReadSingle());
		//minNeighbors
		minNeighbors = (int)(reader.ReadInt32());
		//minObjectHeight
		minObjectHeight = (int)(reader.ReadInt32());
		//minObjectWidth
		minObjectWidth = (int)(reader.ReadInt32());
		//maxObjectHeight
		maxObjectHeight = (int)(reader.ReadInt32());
		//maxObjectWidth
		maxObjectWidth = (int)(reader.ReadInt32());
	}

	/**** Pack ****/
	public System.IO.Stream Pack(System.IO.Stream stream)
	{
		System.IO.BinaryWriter writer = new System.IO.BinaryWriter(stream);
		writer.Write((float)scaleFactor);
		writer.Write((int)minNeighbors);
		writer.Write((int)minObjectHeight);
		writer.Write((int)minObjectWidth);
		writer.Write((int)maxObjectHeight);
		writer.Write((int)maxObjectWidth);
		return stream;
	}

	/**** Unpack ****/
	public System.IO.Stream Unpack(System.IO.Stream stream)
	{
		System.IO.BinaryReader reader = new System.IO.BinaryReader(stream);
		//scaleFactor
		scaleFactor = (float)(reader.ReadSingle());
		//minNeighbors
		minNeighbors = (int)(reader.ReadInt32());
		//minObjectHeight
		minObjectHeight = (int)(reader.ReadInt32());
		//minObjectWidth
		minObjectWidth = (int)(reader.ReadInt32());
		//maxObjectHeight
		maxObjectHeight = (int)(reader.ReadInt32());
		//maxObjectWidth
		maxObjectWidth = (int)(reader.ReadInt32());
		return stream;
	}
	public int Size
	{
		get {
			int result = 0;
			//scaleFactor
			result += 4; // = float_32
			//minNeighbors
			result += 4; // = int_32
			//minObjectHeight
			result += 4; // = int_32
			//minObjectWidth
			result += 4; // = int_32
			//maxObjectHeight
			result += 4; // = int_32
			//maxObjectWidth
			result += 4; // = int_32
			return result;
		}
	}
}

public class U2G_Message {
	public enum Tag {
		Ping,	//0
		ConnectToRobot,	//1
		ConnectToUiDevice,	//2
		DisconnectFromUiDevice,	//3
		ForceAddRobot,	//4
		StartEngine,	//5
		DriveWheels,	//6
		TurnInPlace,	//7
		MoveHead,	//8
		MoveLift,	//9
		SetLiftHeight,	//10
		SetHeadAngle,	//11
		StopAllMotors,	//12
		ImageRequest,	//13
		SetRobotImageSendMode,	//14
		SaveImages,	//15
		EnableDisplay,	//16
		SetHeadlights,	//17
		GotoPose,	//18
		PlaceObjectOnGround,	//19
		PlaceObjectOnGroundHere,	//20
		ExecuteTestPlan,	//21
		SelectNextObject,	//22
		PickAndPlaceObject,	//23
		TraverseObject,	//24
		ClearAllBlocks,	//25
		ExecuteBehavior,	//26
		SetBehaviorState,	//27
		AbortPath,	//28
		AbortAll,	//29
		DrawPoseMarker,	//30
		ErasePoseMarker,	//31
		SetHeadControllerGains,	//32
		SetLiftControllerGains,	//33
		SelectNextSoundScheme,	//34
		StartTestMode,	//35
		IMURequest,	//36
		PlayAnimation,	//37
		ReadAnimationFile,	//38
		StartFaceTracking,	//39
		StopFaceTracking,	//40
		StartLookingForMarkers,	//41
		StopLookingForMarkers,	//42
		SetVisionSystemParams,	//43
		SetFaceDetectParams,	//44
		INVALID
	};

	private Tag _tag = Tag.INVALID;

	public Tag GetTag() { return _tag; }

	private object _state = null;

	public U2G_Ping Ping
	{
		get {
			if (_tag != Tag.Ping) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"Ping\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_Ping)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.Ping : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ConnectToRobot ConnectToRobot
	{
		get {
			if (_tag != Tag.ConnectToRobot) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ConnectToRobot\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ConnectToRobot)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ConnectToRobot : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ConnectToUiDevice ConnectToUiDevice
	{
		get {
			if (_tag != Tag.ConnectToUiDevice) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ConnectToUiDevice\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ConnectToUiDevice)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ConnectToUiDevice : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_DisconnectFromUiDevice DisconnectFromUiDevice
	{
		get {
			if (_tag != Tag.DisconnectFromUiDevice) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"DisconnectFromUiDevice\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_DisconnectFromUiDevice)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.DisconnectFromUiDevice : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ForceAddRobot ForceAddRobot
	{
		get {
			if (_tag != Tag.ForceAddRobot) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ForceAddRobot\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ForceAddRobot)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ForceAddRobot : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StartEngine StartEngine
	{
		get {
			if (_tag != Tag.StartEngine) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StartEngine\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StartEngine)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StartEngine : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_DriveWheels DriveWheels
	{
		get {
			if (_tag != Tag.DriveWheels) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"DriveWheels\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_DriveWheels)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.DriveWheels : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_TurnInPlace TurnInPlace
	{
		get {
			if (_tag != Tag.TurnInPlace) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"TurnInPlace\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_TurnInPlace)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.TurnInPlace : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_MoveHead MoveHead
	{
		get {
			if (_tag != Tag.MoveHead) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"MoveHead\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_MoveHead)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.MoveHead : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_MoveLift MoveLift
	{
		get {
			if (_tag != Tag.MoveLift) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"MoveLift\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_MoveLift)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.MoveLift : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetLiftHeight SetLiftHeight
	{
		get {
			if (_tag != Tag.SetLiftHeight) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetLiftHeight\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetLiftHeight)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetLiftHeight : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetHeadAngle SetHeadAngle
	{
		get {
			if (_tag != Tag.SetHeadAngle) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetHeadAngle\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetHeadAngle)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetHeadAngle : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StopAllMotors StopAllMotors
	{
		get {
			if (_tag != Tag.StopAllMotors) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StopAllMotors\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StopAllMotors)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StopAllMotors : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ImageRequest ImageRequest
	{
		get {
			if (_tag != Tag.ImageRequest) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ImageRequest\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ImageRequest)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ImageRequest : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetRobotImageSendMode SetRobotImageSendMode
	{
		get {
			if (_tag != Tag.SetRobotImageSendMode) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetRobotImageSendMode\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetRobotImageSendMode)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetRobotImageSendMode : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SaveImages SaveImages
	{
		get {
			if (_tag != Tag.SaveImages) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SaveImages\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SaveImages)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SaveImages : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_EnableDisplay EnableDisplay
	{
		get {
			if (_tag != Tag.EnableDisplay) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"EnableDisplay\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_EnableDisplay)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.EnableDisplay : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetHeadlights SetHeadlights
	{
		get {
			if (_tag != Tag.SetHeadlights) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetHeadlights\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetHeadlights)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetHeadlights : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_GotoPose GotoPose
	{
		get {
			if (_tag != Tag.GotoPose) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"GotoPose\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_GotoPose)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.GotoPose : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_PlaceObjectOnGround PlaceObjectOnGround
	{
		get {
			if (_tag != Tag.PlaceObjectOnGround) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"PlaceObjectOnGround\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_PlaceObjectOnGround)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.PlaceObjectOnGround : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_PlaceObjectOnGroundHere PlaceObjectOnGroundHere
	{
		get {
			if (_tag != Tag.PlaceObjectOnGroundHere) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"PlaceObjectOnGroundHere\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_PlaceObjectOnGroundHere)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.PlaceObjectOnGroundHere : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ExecuteTestPlan ExecuteTestPlan
	{
		get {
			if (_tag != Tag.ExecuteTestPlan) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ExecuteTestPlan\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ExecuteTestPlan)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ExecuteTestPlan : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SelectNextObject SelectNextObject
	{
		get {
			if (_tag != Tag.SelectNextObject) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SelectNextObject\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SelectNextObject)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SelectNextObject : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_PickAndPlaceObject PickAndPlaceObject
	{
		get {
			if (_tag != Tag.PickAndPlaceObject) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"PickAndPlaceObject\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_PickAndPlaceObject)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.PickAndPlaceObject : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_TraverseObject TraverseObject
	{
		get {
			if (_tag != Tag.TraverseObject) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"TraverseObject\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_TraverseObject)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.TraverseObject : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ClearAllBlocks ClearAllBlocks
	{
		get {
			if (_tag != Tag.ClearAllBlocks) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ClearAllBlocks\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ClearAllBlocks)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ClearAllBlocks : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ExecuteBehavior ExecuteBehavior
	{
		get {
			if (_tag != Tag.ExecuteBehavior) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ExecuteBehavior\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ExecuteBehavior)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ExecuteBehavior : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetBehaviorState SetBehaviorState
	{
		get {
			if (_tag != Tag.SetBehaviorState) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetBehaviorState\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetBehaviorState)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetBehaviorState : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_AbortPath AbortPath
	{
		get {
			if (_tag != Tag.AbortPath) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"AbortPath\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_AbortPath)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.AbortPath : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_AbortAll AbortAll
	{
		get {
			if (_tag != Tag.AbortAll) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"AbortAll\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_AbortAll)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.AbortAll : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_DrawPoseMarker DrawPoseMarker
	{
		get {
			if (_tag != Tag.DrawPoseMarker) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"DrawPoseMarker\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_DrawPoseMarker)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.DrawPoseMarker : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ErasePoseMarker ErasePoseMarker
	{
		get {
			if (_tag != Tag.ErasePoseMarker) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ErasePoseMarker\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ErasePoseMarker)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ErasePoseMarker : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetHeadControllerGains SetHeadControllerGains
	{
		get {
			if (_tag != Tag.SetHeadControllerGains) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetHeadControllerGains\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetHeadControllerGains)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetHeadControllerGains : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetLiftControllerGains SetLiftControllerGains
	{
		get {
			if (_tag != Tag.SetLiftControllerGains) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetLiftControllerGains\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetLiftControllerGains)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetLiftControllerGains : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SelectNextSoundScheme SelectNextSoundScheme
	{
		get {
			if (_tag != Tag.SelectNextSoundScheme) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SelectNextSoundScheme\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SelectNextSoundScheme)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SelectNextSoundScheme : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StartTestMode StartTestMode
	{
		get {
			if (_tag != Tag.StartTestMode) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StartTestMode\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StartTestMode)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StartTestMode : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_IMURequest IMURequest
	{
		get {
			if (_tag != Tag.IMURequest) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"IMURequest\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_IMURequest)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.IMURequest : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_PlayAnimation PlayAnimation
	{
		get {
			if (_tag != Tag.PlayAnimation) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"PlayAnimation\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_PlayAnimation)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.PlayAnimation : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_ReadAnimationFile ReadAnimationFile
	{
		get {
			if (_tag != Tag.ReadAnimationFile) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"ReadAnimationFile\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_ReadAnimationFile)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.ReadAnimationFile : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StartFaceTracking StartFaceTracking
	{
		get {
			if (_tag != Tag.StartFaceTracking) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StartFaceTracking\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StartFaceTracking)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StartFaceTracking : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StopFaceTracking StopFaceTracking
	{
		get {
			if (_tag != Tag.StopFaceTracking) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StopFaceTracking\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StopFaceTracking)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StopFaceTracking : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StartLookingForMarkers StartLookingForMarkers
	{
		get {
			if (_tag != Tag.StartLookingForMarkers) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StartLookingForMarkers\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StartLookingForMarkers)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StartLookingForMarkers : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_StopLookingForMarkers StopLookingForMarkers
	{
		get {
			if (_tag != Tag.StopLookingForMarkers) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"StopLookingForMarkers\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_StopLookingForMarkers)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.StopLookingForMarkers : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetVisionSystemParams SetVisionSystemParams
	{
		get {
			if (_tag != Tag.SetVisionSystemParams) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetVisionSystemParams\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetVisionSystemParams)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetVisionSystemParams : Tag.INVALID;
			_state = value;
		}
	}

	public U2G_SetFaceDetectParams SetFaceDetectParams
	{
		get {
			if (_tag != Tag.SetFaceDetectParams) {
				throw new System.InvalidOperationException(string.Format(
					"Cannot access union member \"SetFaceDetectParams\" when a value of type {0} is stored.",
					_tag.ToString()));
			}
			return (U2G_SetFaceDetectParams)this._state;
		}
		
		set {
			_tag = (value != null) ? Tag.SetFaceDetectParams : Tag.INVALID;
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
			_state = new U2G_Ping(stream);
			break;
		case Tag.ConnectToRobot:
			_state = new U2G_ConnectToRobot(stream);
			break;
		case Tag.ConnectToUiDevice:
			_state = new U2G_ConnectToUiDevice(stream);
			break;
		case Tag.DisconnectFromUiDevice:
			_state = new U2G_DisconnectFromUiDevice(stream);
			break;
		case Tag.ForceAddRobot:
			_state = new U2G_ForceAddRobot(stream);
			break;
		case Tag.StartEngine:
			_state = new U2G_StartEngine(stream);
			break;
		case Tag.DriveWheels:
			_state = new U2G_DriveWheels(stream);
			break;
		case Tag.TurnInPlace:
			_state = new U2G_TurnInPlace(stream);
			break;
		case Tag.MoveHead:
			_state = new U2G_MoveHead(stream);
			break;
		case Tag.MoveLift:
			_state = new U2G_MoveLift(stream);
			break;
		case Tag.SetLiftHeight:
			_state = new U2G_SetLiftHeight(stream);
			break;
		case Tag.SetHeadAngle:
			_state = new U2G_SetHeadAngle(stream);
			break;
		case Tag.StopAllMotors:
			_state = new U2G_StopAllMotors(stream);
			break;
		case Tag.ImageRequest:
			_state = new U2G_ImageRequest(stream);
			break;
		case Tag.SetRobotImageSendMode:
			_state = new U2G_SetRobotImageSendMode(stream);
			break;
		case Tag.SaveImages:
			_state = new U2G_SaveImages(stream);
			break;
		case Tag.EnableDisplay:
			_state = new U2G_EnableDisplay(stream);
			break;
		case Tag.SetHeadlights:
			_state = new U2G_SetHeadlights(stream);
			break;
		case Tag.GotoPose:
			_state = new U2G_GotoPose(stream);
			break;
		case Tag.PlaceObjectOnGround:
			_state = new U2G_PlaceObjectOnGround(stream);
			break;
		case Tag.PlaceObjectOnGroundHere:
			_state = new U2G_PlaceObjectOnGroundHere(stream);
			break;
		case Tag.ExecuteTestPlan:
			_state = new U2G_ExecuteTestPlan(stream);
			break;
		case Tag.SelectNextObject:
			_state = new U2G_SelectNextObject(stream);
			break;
		case Tag.PickAndPlaceObject:
			_state = new U2G_PickAndPlaceObject(stream);
			break;
		case Tag.TraverseObject:
			_state = new U2G_TraverseObject(stream);
			break;
		case Tag.ClearAllBlocks:
			_state = new U2G_ClearAllBlocks(stream);
			break;
		case Tag.ExecuteBehavior:
			_state = new U2G_ExecuteBehavior(stream);
			break;
		case Tag.SetBehaviorState:
			_state = new U2G_SetBehaviorState(stream);
			break;
		case Tag.AbortPath:
			_state = new U2G_AbortPath(stream);
			break;
		case Tag.AbortAll:
			_state = new U2G_AbortAll(stream);
			break;
		case Tag.DrawPoseMarker:
			_state = new U2G_DrawPoseMarker(stream);
			break;
		case Tag.ErasePoseMarker:
			_state = new U2G_ErasePoseMarker(stream);
			break;
		case Tag.SetHeadControllerGains:
			_state = new U2G_SetHeadControllerGains(stream);
			break;
		case Tag.SetLiftControllerGains:
			_state = new U2G_SetLiftControllerGains(stream);
			break;
		case Tag.SelectNextSoundScheme:
			_state = new U2G_SelectNextSoundScheme(stream);
			break;
		case Tag.StartTestMode:
			_state = new U2G_StartTestMode(stream);
			break;
		case Tag.IMURequest:
			_state = new U2G_IMURequest(stream);
			break;
		case Tag.PlayAnimation:
			_state = new U2G_PlayAnimation(stream);
			break;
		case Tag.ReadAnimationFile:
			_state = new U2G_ReadAnimationFile(stream);
			break;
		case Tag.StartFaceTracking:
			_state = new U2G_StartFaceTracking(stream);
			break;
		case Tag.StopFaceTracking:
			_state = new U2G_StopFaceTracking(stream);
			break;
		case Tag.StartLookingForMarkers:
			_state = new U2G_StartLookingForMarkers(stream);
			break;
		case Tag.StopLookingForMarkers:
			_state = new U2G_StopLookingForMarkers(stream);
			break;
		case Tag.SetVisionSystemParams:
			_state = new U2G_SetVisionSystemParams(stream);
			break;
		case Tag.SetFaceDetectParams:
			_state = new U2G_SetFaceDetectParams(stream);
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
		case Tag.ConnectToRobot:
			ConnectToRobot.Pack(stream);
			break;
		case Tag.ConnectToUiDevice:
			ConnectToUiDevice.Pack(stream);
			break;
		case Tag.DisconnectFromUiDevice:
			DisconnectFromUiDevice.Pack(stream);
			break;
		case Tag.ForceAddRobot:
			ForceAddRobot.Pack(stream);
			break;
		case Tag.StartEngine:
			StartEngine.Pack(stream);
			break;
		case Tag.DriveWheels:
			DriveWheels.Pack(stream);
			break;
		case Tag.TurnInPlace:
			TurnInPlace.Pack(stream);
			break;
		case Tag.MoveHead:
			MoveHead.Pack(stream);
			break;
		case Tag.MoveLift:
			MoveLift.Pack(stream);
			break;
		case Tag.SetLiftHeight:
			SetLiftHeight.Pack(stream);
			break;
		case Tag.SetHeadAngle:
			SetHeadAngle.Pack(stream);
			break;
		case Tag.StopAllMotors:
			StopAllMotors.Pack(stream);
			break;
		case Tag.ImageRequest:
			ImageRequest.Pack(stream);
			break;
		case Tag.SetRobotImageSendMode:
			SetRobotImageSendMode.Pack(stream);
			break;
		case Tag.SaveImages:
			SaveImages.Pack(stream);
			break;
		case Tag.EnableDisplay:
			EnableDisplay.Pack(stream);
			break;
		case Tag.SetHeadlights:
			SetHeadlights.Pack(stream);
			break;
		case Tag.GotoPose:
			GotoPose.Pack(stream);
			break;
		case Tag.PlaceObjectOnGround:
			PlaceObjectOnGround.Pack(stream);
			break;
		case Tag.PlaceObjectOnGroundHere:
			PlaceObjectOnGroundHere.Pack(stream);
			break;
		case Tag.ExecuteTestPlan:
			ExecuteTestPlan.Pack(stream);
			break;
		case Tag.SelectNextObject:
			SelectNextObject.Pack(stream);
			break;
		case Tag.PickAndPlaceObject:
			PickAndPlaceObject.Pack(stream);
			break;
		case Tag.TraverseObject:
			TraverseObject.Pack(stream);
			break;
		case Tag.ClearAllBlocks:
			ClearAllBlocks.Pack(stream);
			break;
		case Tag.ExecuteBehavior:
			ExecuteBehavior.Pack(stream);
			break;
		case Tag.SetBehaviorState:
			SetBehaviorState.Pack(stream);
			break;
		case Tag.AbortPath:
			AbortPath.Pack(stream);
			break;
		case Tag.AbortAll:
			AbortAll.Pack(stream);
			break;
		case Tag.DrawPoseMarker:
			DrawPoseMarker.Pack(stream);
			break;
		case Tag.ErasePoseMarker:
			ErasePoseMarker.Pack(stream);
			break;
		case Tag.SetHeadControllerGains:
			SetHeadControllerGains.Pack(stream);
			break;
		case Tag.SetLiftControllerGains:
			SetLiftControllerGains.Pack(stream);
			break;
		case Tag.SelectNextSoundScheme:
			SelectNextSoundScheme.Pack(stream);
			break;
		case Tag.StartTestMode:
			StartTestMode.Pack(stream);
			break;
		case Tag.IMURequest:
			IMURequest.Pack(stream);
			break;
		case Tag.PlayAnimation:
			PlayAnimation.Pack(stream);
			break;
		case Tag.ReadAnimationFile:
			ReadAnimationFile.Pack(stream);
			break;
		case Tag.StartFaceTracking:
			StartFaceTracking.Pack(stream);
			break;
		case Tag.StopFaceTracking:
			StopFaceTracking.Pack(stream);
			break;
		case Tag.StartLookingForMarkers:
			StartLookingForMarkers.Pack(stream);
			break;
		case Tag.StopLookingForMarkers:
			StopLookingForMarkers.Pack(stream);
			break;
		case Tag.SetVisionSystemParams:
			SetVisionSystemParams.Pack(stream);
			break;
		case Tag.SetFaceDetectParams:
			SetFaceDetectParams.Pack(stream);
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
			case Tag.ConnectToRobot:
				result += ConnectToRobot.Size;
				break;
			case Tag.ConnectToUiDevice:
				result += ConnectToUiDevice.Size;
				break;
			case Tag.DisconnectFromUiDevice:
				result += DisconnectFromUiDevice.Size;
				break;
			case Tag.ForceAddRobot:
				result += ForceAddRobot.Size;
				break;
			case Tag.StartEngine:
				result += StartEngine.Size;
				break;
			case Tag.DriveWheels:
				result += DriveWheels.Size;
				break;
			case Tag.TurnInPlace:
				result += TurnInPlace.Size;
				break;
			case Tag.MoveHead:
				result += MoveHead.Size;
				break;
			case Tag.MoveLift:
				result += MoveLift.Size;
				break;
			case Tag.SetLiftHeight:
				result += SetLiftHeight.Size;
				break;
			case Tag.SetHeadAngle:
				result += SetHeadAngle.Size;
				break;
			case Tag.StopAllMotors:
				result += StopAllMotors.Size;
				break;
			case Tag.ImageRequest:
				result += ImageRequest.Size;
				break;
			case Tag.SetRobotImageSendMode:
				result += SetRobotImageSendMode.Size;
				break;
			case Tag.SaveImages:
				result += SaveImages.Size;
				break;
			case Tag.EnableDisplay:
				result += EnableDisplay.Size;
				break;
			case Tag.SetHeadlights:
				result += SetHeadlights.Size;
				break;
			case Tag.GotoPose:
				result += GotoPose.Size;
				break;
			case Tag.PlaceObjectOnGround:
				result += PlaceObjectOnGround.Size;
				break;
			case Tag.PlaceObjectOnGroundHere:
				result += PlaceObjectOnGroundHere.Size;
				break;
			case Tag.ExecuteTestPlan:
				result += ExecuteTestPlan.Size;
				break;
			case Tag.SelectNextObject:
				result += SelectNextObject.Size;
				break;
			case Tag.PickAndPlaceObject:
				result += PickAndPlaceObject.Size;
				break;
			case Tag.TraverseObject:
				result += TraverseObject.Size;
				break;
			case Tag.ClearAllBlocks:
				result += ClearAllBlocks.Size;
				break;
			case Tag.ExecuteBehavior:
				result += ExecuteBehavior.Size;
				break;
			case Tag.SetBehaviorState:
				result += SetBehaviorState.Size;
				break;
			case Tag.AbortPath:
				result += AbortPath.Size;
				break;
			case Tag.AbortAll:
				result += AbortAll.Size;
				break;
			case Tag.DrawPoseMarker:
				result += DrawPoseMarker.Size;
				break;
			case Tag.ErasePoseMarker:
				result += ErasePoseMarker.Size;
				break;
			case Tag.SetHeadControllerGains:
				result += SetHeadControllerGains.Size;
				break;
			case Tag.SetLiftControllerGains:
				result += SetLiftControllerGains.Size;
				break;
			case Tag.SelectNextSoundScheme:
				result += SelectNextSoundScheme.Size;
				break;
			case Tag.StartTestMode:
				result += StartTestMode.Size;
				break;
			case Tag.IMURequest:
				result += IMURequest.Size;
				break;
			case Tag.PlayAnimation:
				result += PlayAnimation.Size;
				break;
			case Tag.ReadAnimationFile:
				result += ReadAnimationFile.Size;
				break;
			case Tag.StartFaceTracking:
				result += StartFaceTracking.Size;
				break;
			case Tag.StopFaceTracking:
				result += StopFaceTracking.Size;
				break;
			case Tag.StartLookingForMarkers:
				result += StartLookingForMarkers.Size;
				break;
			case Tag.StopLookingForMarkers:
				result += StopLookingForMarkers.Size;
				break;
			case Tag.SetVisionSystemParams:
				result += SetVisionSystemParams.Size;
				break;
			case Tag.SetFaceDetectParams:
				result += SetFaceDetectParams.Size;
				break;
			default:
				return 0;
			}
			return result;
		}
	}
}

}//namespace Anki.Cozmo
