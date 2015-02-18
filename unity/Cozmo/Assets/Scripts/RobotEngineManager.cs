
using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

public class RobotEngineManager : MonoBehaviour {
	
	public static RobotEngineManager instance = null;

	public Dictionary<int, Robot> robots { get; private set; }
	
	public Robot current { get { return robots[ Intro.CurrentRobotID ]; } }
	
	public bool IsConnected {
		get {
			return (channel != null && channel.IsConnected);
		}
	}
	
	[SerializeField]
	private TextAsset configuration;
	
	public event Action<string> ConnectedToClient;
	public event Action<DisconnectionReason> DisconnectedFromClient;
	public event Action<int> RobotConnected;
	public event Action<Texture2D> RobotImage; 
	
	private ChannelBase channel;
	
	private const int UIDeviceID = 1;
	private const int UIAdvertisingRegistrationPort = 5103;
	private const int UILocalPort = 5106;
	
	#if !UNITY_EDITOR
	private bool engineHostInitialized = false;
	
	private StringBuilder logBuilder = null;
	
	private void Start()
	{
		if (engineHostInitialized) {
			Debug.LogError("Error initializing cozmo host: Host already started.");
			return;
		}
		
		if (configuration == null) {
			Debug.LogError("Error initializing cozmo host: No configuration.");
			return;
		}
		
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_game_create (configuration.text);
		
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_engine_create error: " + result.ToString());
		} else {
			engineHostInitialized = true;
		}
	}
	
	private void OnDestroy()
	{
		if (engineHostInitialized) {
			engineHostInitialized = false;
			
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_game_destroy();
			if (result != CozmoResult.OK) {
				Debug.LogError("cozmo_engine_destroy error: " + result.ToString());
			}
		}
	}
	
	// should be update; but this makes it easier to split up
	private void LateUpdate()
	{
		if (logBuilder == null) {
			logBuilder = new StringBuilder (1024);
		}
		
		int length;
		while (CozmoBinding.cozmo_has_log(out length)) {
			if (logBuilder.Capacity < length) {
				logBuilder.Capacity = Math.Max (logBuilder.Capacity * 2, length);
			}
			
			CozmoBinding.cozmo_pop_log (logBuilder, logBuilder.Capacity);
			Debug.LogError (logBuilder.ToString ());
		}
		
		if (engineHostInitialized) {
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_game_update (Time.realtimeSinceStartup);
			if (result != CozmoResult.OK) {
				Debug.LogError ("cozmo_engine_update error: " + result.ToString ());
			}
		}
	}
	#endif
	
	private void Awake() {
		if (instance != null) {
			Destroy (gameObject);
		} else {
			instance = this;
			DontDestroyOnLoad (gameObject);
		}
		
		Application.runInBackground = true;

		robots = new Dictionary<int, Robot>();

		AddRobot( Intro.CurrentRobotID );
	}
	
	public void AddRobot( byte robotID )
	{
		robots.Add( robotID, new Robot( robotID ) );
	}
	
	private void OnEnable()
	{
		channel = new UdpChannel ();
		channel.ConnectedToClient += Connected;
		channel.DisconnectedFromClient += Disconnected;
		channel.MessageReceived += ReceivedMessage;
	}
	
	private void OnDisable()
	{
		if (channel != null) {
			Disconnect ();
			channel = null;
		}
	}
	
	private void Update()
	{
		if (channel != null) {
			channel.Update ();
		}
	}
	
	public void Connect(string engineIP)
	{
		channel.Connect (UIDeviceID, UILocalPort, engineIP, UIAdvertisingRegistrationPort);
	}
	
	public void Disconnect()
	{
		if (channel != null) {
			channel.Disconnect ();
			
			#if UNITY_EDITOR
			float limit = Time.realtimeSinceStartup + 2.0f;
			while (channel.HasPendingOperations) {
				if (limit < Time.realtimeSinceStartup) {
					Debug.LogWarning("Not waiting for disconnect to finish sending.");
					break;
				}
				System.Threading.Thread.Sleep (500);
			}
			#endif
		}
	}
	
	private void Connected(string connectionIdentifier)
	{
		if (ConnectedToClient != null) {
			ConnectedToClient(connectionIdentifier);
		}
	}
	
	private void Disconnected(DisconnectionReason reason)
	{
		Application.LoadLevel ("Shell");
		
		if (DisconnectedFromClient != null) {
			DisconnectedFromClient(reason);
		}
	}
	
	private void ReceivedMessage(NetworkMessage message)
	{
		switch (message.ID) {
		case (int)NetworkMessageID.G2U_RobotAvailable:
			ReceivedSpecificMessage((G2U_RobotAvailable)message);
			break;
		case (int)NetworkMessageID.G2U_UiDeviceAvailable:
			ReceivedSpecificMessage((G2U_UiDeviceAvailable)message);
			break;
		case (int)NetworkMessageID.G2U_RobotConnected:
			ReceivedSpecificMessage((G2U_RobotConnected)message);
			break;
		case (int)NetworkMessageID.G2U_UiDeviceConnected:
			ReceivedSpecificMessage((G2U_UiDeviceConnected)message);
			break;
		case (int)NetworkMessageID.G2U_RobotDisconnected:
			ReceivedSpecificMessage((G2U_RobotDisconnected)message);
			break;
		case (int)NetworkMessageID.G2U_RobotObservedObject:
			ReceivedSpecificMessage((G2U_RobotObservedObject)message);
			break;
		case (int)NetworkMessageID.G2U_RobotObservedNothing:
			ReceivedSpecificMessage((G2U_RobotObservedNothing)message);
			break;
		case (int)NetworkMessageID.G2U_DeviceDetectedVisionMarker:
			ReceivedSpecificMessage((G2U_DeviceDetectedVisionMarker)message);
			break;
		case (int)NetworkMessageID.G2U_PlaySound:
			ReceivedSpecificMessage((G2U_PlaySound)message);
			break;
		case (int)NetworkMessageID.G2U_StopSound:
			ReceivedSpecificMessage((G2U_StopSound)message);
			break;
		case (int)NetworkMessageID.G2U_ImageChunk:
			ReceivedSpecificMessage((G2U_ImageChunk)message);
			break;
		case (int)NetworkMessageID.G2U_RobotState:
			ReceivedSpecificMessage((G2U_RobotState)message);
			break;
		}
	}
	
	private void ReceivedSpecificMessage(G2U_RobotAvailable message)
	{
		U2G_ConnectToRobot response = new U2G_ConnectToRobot();
		response.robotID = (byte)message.robotID;
		
		channel.Send (response);
	}
	
	private void ReceivedSpecificMessage(G2U_UiDeviceAvailable message)
	{
		U2G_ConnectToUiDevice response = new U2G_ConnectToUiDevice ();
		response.deviceID = (byte)message.deviceID;
		
		channel.Send (response);
	}
	
	private void ReceivedSpecificMessage(G2U_RobotConnected message)
	{
		if (RobotConnected != null) {
			RobotConnected((int)message.robotID);
		}
	}
	
	private void ReceivedSpecificMessage(G2U_UiDeviceConnected message)
	{
		Debug.Log ("Device connected: " + message.deviceID.ToString());
	}
	
	private void ReceivedSpecificMessage(G2U_RobotDisconnected message)
	{
		Debug.LogError ("Robot " + message.robotID + " disconnected after " + message.timeSinceLastMsg_sec.ToString ("0.2f") + " seconds.");
		Disconnected (DisconnectionReason.RobotDisconnected);
	}
	
	private void ReceivedSpecificMessage(G2U_RobotObservedObject message)
	{
		Debug.Log( "box found " + Time.time );

		current.box.UpdateInfo( message );
	}

	private void ReceivedSpecificMessage( G2U_RobotObservedNothing message )
	{
		Debug.Log( "no box found " + Time.time );

		current.box.RemoveInfo();
	}
	
	private void ReceivedSpecificMessage(G2U_DeviceDetectedVisionMarker message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_PlaySound message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_StopSound message)
	{
		
	}
	
	private void ReceivedSpecificMessage( G2U_RobotState message )
	{
		if( !robots.ContainsKey( message.robotID ) )
		{
			Debug.Log( "adding robot with ID: " + message.robotID );
			
			AddRobot( message.robotID );
		}
		
		robots[ message.robotID ].UpdateInfo( message );
	}
	
	private Texture2D texture;
	private int currentImageIndex;
	private UInt32 currentImageID = UInt32.MaxValue;
	private UInt32 currentImageFrameTimeStamp = UInt32.MaxValue;
	private Color32[] colorArray;
	
	private void ReceivedSpecificMessage( G2U_ImageChunk message )
	{
		if( colorArray == null || message.imageId != currentImageID || message.frameTimeStamp != currentImageFrameTimeStamp )
		{
			currentImageID = message.imageId;
			currentImageFrameTimeStamp = message.frameTimeStamp;
			
			int length = message.ncols * message.nrows;
			
			if( colorArray == null || colorArray.Length != length )
			{
				colorArray = new Color32[ length ];
			}
			
			currentImageIndex = 0;
		}
		
		for( int messageIndex = 0; currentImageIndex < colorArray.Length && messageIndex < message.chunkSize; ++messageIndex, ++currentImageIndex )
		{
			byte gray = message.data[ messageIndex ];
			
			int x = currentImageIndex % message.ncols;
			int y = currentImageIndex / message.ncols;
			int index = message.ncols * ( message.nrows - y - 1 ) + x;
			
			colorArray[ index ] = new Color32( gray, gray, gray, 255 );
		}
		
		if( currentImageIndex == colorArray.Length )
		{
			int width = message.ncols;
			int height = message.nrows;
			
			if( texture != null )
			{
				if( texture.width != width || texture.height != height )
				{
					Destroy( texture );
					texture = null;
				}
			}
			
			if( texture == null )
			{
				texture = new Texture2D( width, height, TextureFormat.ARGB32, false );
			}
			
			texture.SetPixels32( colorArray );
			
			texture.Apply( false );
			
			if( RobotImage != null )
			{
				RobotImage( texture );
			}
		}
	}
	
	public void StartEngine(string vizHostIP)
	{
		U2G_StartEngine message = new U2G_StartEngine ();
		message.asHost = 1;
		int length = 0;
		if (!string.IsNullOrEmpty (vizHostIP)) {
			length = Encoding.UTF8.GetByteCount(vizHostIP);
			if (length + 1 > message.vizHostIP.Length) {
				throw new ArgumentException("vizHostIP is too long. (" + (length + 1).ToString() + " bytes provided, max " + message.vizHostIP.Length + ".)");
			}
			Encoding.UTF8.GetBytes (vizHostIP, 0, vizHostIP.Length, message.vizHostIP, 0);
		}
		message.vizHostIP [length] = 0;
		
		channel.Send (message);
	}
	
	/// <summary>
	/// Forcibly adds a new robot.
	/// </summary>
	/// <param name="robotId">The robot identifier.</param>
	/// <param name="robotIP">The ip address the robot is connected to.</param>
	/// <param name="robotIsSimulated">Specify true for a simulated robot.</param>
	public void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated)
	{
		if (robotID < 0 || robotID > 255) {
			throw new ArgumentException("ID must be between 0 and 255.", "robotID");
		}
		
		if (string.IsNullOrEmpty (robotIP)) {
			throw new ArgumentNullException("robotIP");
		}
		
		U2G_ForceAddRobot message = new U2G_ForceAddRobot ();
		if (Encoding.UTF8.GetByteCount (robotIP) + 1 > message.ipAddress.Length) {
			throw new ArgumentException("IP address too long.", "robotIP");
		}
		int length = Encoding.UTF8.GetBytes (robotIP, 0, robotIP.Length, message.ipAddress, 0);
		message.ipAddress [length] = 0;
		
		message.robotID = (byte)robotID;
		message.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;
		
		channel.Send (message);
	}
	
	/// <summary>
	/// Set wheel speed.
	/// </summary>
	/// <param name="left_wheel_speed_mmps">Left wheel speed in millimeters per second.</param>
	/// <param name="right_wheel_speed_mmps">Right wheel speed in millimeters per second.</param>
	public void DriveWheels(int robotID, float leftWheelSpeedMmps, float rightWheelSpeedMmps)
	{
		if (robotID < 0 || robotID > 255) {
			throw new ArgumentException("ID must be between 0 and 255.", "robotID");
		}
		
		U2G_DriveWheels message = new U2G_DriveWheels ();
		message.lwheel_speed_mmps = leftWheelSpeedMmps;
		message.rwheel_speed_mmps = rightWheelSpeedMmps;
		
		channel.Send (message);
	}
	
	public enum ImageSendMode_t
	{
		ISM_OFF,
		ISM_STREAM,
		ISM_SINGLE_SHOT
	} ;
	
	public enum CameraResolution
	{
		CAMERA_RES_QUXGA = 0, // 3200 x 2400
		CAMERA_RES_QXGA,      // 2048 x 1536
		CAMERA_RES_UXGA,      // 1600 x 1200
		CAMERA_RES_SXGA,      // 1280 x 960, technically SXGA-
		CAMERA_RES_XGA,       // 1024 x 768
		CAMERA_RES_SVGA,      // 800 x 600
		CAMERA_RES_VGA,       // 640 x 480
		CAMERA_RES_QVGA,      // 320 x 240
		CAMERA_RES_QQVGA,     // 160 x 120
		CAMERA_RES_QQQVGA,    // 80 x 60
		CAMERA_RES_QQQQVGA,   // 40 x 30
		CAMERA_RES_VERIFICATION_SNAPSHOT, // 16 x 16
		CAMERA_RES_COUNT,
		CAMERA_RES_NONE = CAMERA_RES_COUNT
	} 

	public void PickUpBox()
	{
		U2G_PickAndPlaceObject message = new U2G_PickAndPlaceObject();
		message.objectID = (int)current.box.ID;
		message.usePreDockPose = 0;
		
		channel.Send( message );
	}

	public void RequestImage(int robotID)
	{
		if (robotID < 0 || robotID > 255) {
			throw new ArgumentException("ID must be between 0 and 255.", "robotID");
		}
		
		U2G_SetRobotImageSendMode message = new U2G_SetRobotImageSendMode ();
		message.resolution = (byte)CameraResolution.CAMERA_RES_QVGA;
		message.mode = (byte)ImageSendMode_t.ISM_STREAM;
		
		channel.Send (message);
		
		U2G_ImageRequest message2 = new U2G_ImageRequest ();
		message2.robotID = (byte)robotID;
		message2.mode = (byte)ImageSendMode_t.ISM_STREAM;
		
		channel.Send (message2);
		
		Debug.Log( "image request message sent" );
	}
	
	public void StopAllMotors(int robotID)
	{
		if (robotID < 0 || robotID > 255) {
			throw new ArgumentException("ID must be between 0 and 255.", "robotID");
		}
		
		U2G_StopAllMotors message = new U2G_StopAllMotors ();
		
		channel.Send (message);
	}
	
}
