
using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;
using System.Collections.Generic;
using System.IO;
using System.Text;
using Anki.Cozmo;
using G2U = Anki.Cozmo.G2U;
using U2G = Anki.Cozmo.U2G;

public class RobotEngineManager : MonoBehaviour {
	
	public static RobotEngineManager instance = null;
	
	public Dictionary<int, Robot> robots { get; private set; }
	public List<Robot> robotList = new List<Robot>();
	
	public Robot current { get { return robots[ Intro.CurrentRobotID ]; } }

	public bool IsConnected { get { return (channel != null && channel.IsConnected); } }
	
	[SerializeField] private TextAsset configuration;
	[SerializeField] private GameObject robotBusyPanelPrefab;

	private CozmoBusyPanel busyPanel = null;

	[SerializeField]
	[HideInInspector]
	private DisconnectionReason lastDisconnectionReason = DisconnectionReason.None;

	public event Action<string> ConnectedToClient;
	public event Action<DisconnectionReason> DisconnectedFromClient;
	public event Action<int> RobotConnected;
	public event Action<Texture2D> RobotImage;
	public event Action<bool,RobotActionType> SuccessOrFailure;

	public ChannelBase channel { get; private set; }
	private float lastRobotStateMessage = 0;
	private bool isRobotConnected = false;

	private const int UIDeviceID = 1;
	private const int UIAdvertisingRegistrationPort = 5103;
	private const int UILocalPort = 5106;

	private object sync = new object();
	private List<object> safeLogs = new List<object>();

	public bool AllowImageSaving { get; private set; }
	private bool imageDirectoryCreated = false;

	private const int imageFrameSkip = 0;
	private int imageFrameCount = 0;

	private string imageBasePath;
	private AsyncCallback EndSave_callback;
#if !UNITY_EDITOR
	private bool engineHostInitialized = false;
	
	private StringBuilder logBuilder = null;
	
	private void Start()
	{
		if (engineHostInitialized) {
			Debug.LogError("Error initializing cozmo host: Host already started.", this);
			return;
		}
		
		if (configuration == null) {
			Debug.LogError("Error initializing cozmo host: No configuration.", this);
			return;
		}
		
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_game_create (configuration.text);
		
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_engine_create error: " + result.ToString(), this);
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
				Debug.LogError("cozmo_engine_destroy error: " + result.ToString(), this);
			}
		}
	}
#endif
	private void Awake()
	{
		EndSave_callback = EndSave;
	}

	public void ToggleVisionRecording(bool on) {
		AllowImageSaving = on;

		if(on && !imageDirectoryCreated) {

			imageBasePath = Path.Combine (Application.persistentDataPath, DateTime.Now.ToString ("robovi\\sion_yyyy-MM-dd_HH-mm-ss"));
			try {

				Debug.Log ("Saving robot screenshots to \"" + imageBasePath + "\"", this);
				Directory.CreateDirectory (imageBasePath);

				imageDirectoryCreated = true;

			} catch (Exception e) {

				AllowImageSaving = false;
				Debug.LogException (e, this);
			}
		}
	}

	public void AddRobot( byte robotID )
	{
		Robot robot = new Robot(robotID);
		robots.Add( robotID, robot );
		robotList.Add(robot);
	}
	
	private void OnEnable()
	{
		if (instance != null && instance != this) {
			Destroy (gameObject);
			return;
		} else {
			instance = this;
			DontDestroyOnLoad (gameObject);
		}

		if(busyPanel == null && robotBusyPanelPrefab != null) {
			GameObject busyPanelObject = (GameObject)GameObject.Instantiate(robotBusyPanelPrefab);
			busyPanel = busyPanelObject.GetComponent<CozmoBusyPanel>();
		}

		Application.runInBackground = true;

		channel = new UdpChannel ();
		channel.ConnectedToClient += Connected;
		channel.DisconnectedFromClient += Disconnected;
		channel.MessageReceived += ReceivedMessage;

		robots = new Dictionary<int, Robot>();
		AddRobot( Intro.CurrentRobotID );
	}

	private void OnDisable()
	{
		if (channel != null) {
			if (channel.IsActive) {
				Disconnect ();
				Disconnected (DisconnectionReason.UnityReloaded);
			}

			channel = null;
		}
	}
	 
	private void Update()
	{

		if(Input.GetKeyDown(KeyCode.Mouse3))
			Debug.Break();


		if (channel != null) {
			channel.Update ();
		}

		for(int i=0; i<robotList.Count; i++) {
			robotList[i].CooldownTimers(Time.deltaTime);
		}

		float robotStateTimeout = 20f;
		if (isRobotConnected && lastRobotStateMessage + robotStateTimeout < Time.realtimeSinceStartup) {
			Debug.LogError ("No robot state for " + robotStateTimeout.ToString("0.00") + " seconds.", this);
			Disconnect ();
			Disconnected (DisconnectionReason.RobotConnectionTimedOut);
		}

		// logging from async IO thread
		lock (sync) {
			if (safeLogs.Count > 0) {
				for (int i = 0; i < safeLogs.Count; ++i) {
					object log = safeLogs[i];
					if (log is Exception) {
						Debug.LogException((Exception)log, this);
					}
					else {
						Debug.LogError(log, this);
					}
				}
			}
		}
#if !UNITY_EDITOR
		if (logBuilder == null) {
			logBuilder = new StringBuilder (1024);
		}
		
		int length;
		while (CozmoBinding.cozmo_has_log(out length)) {
			if (logBuilder.Capacity < length) {
				logBuilder.Capacity = Math.Max (logBuilder.Capacity * 2, length);
			}
			
			CozmoBinding.cozmo_pop_log (logBuilder, logBuilder.Capacity);
			Debug.LogError (logBuilder.ToString (), this);
		}
		
		if (engineHostInitialized) {
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_game_update (Time.realtimeSinceStartup);
			if (result != CozmoResult.OK) {
				Debug.LogError ("cozmo_engine_update error: " + result.ToString(), this);
			}
		}
#endif
	}

	public void LateUpdate()
	{
		if( current == null ) return;

		current.UpdateLightMessages();
	}
	
	public void Connect(string engineIP)
	{
		channel.Connect (UIDeviceID, UILocalPort, engineIP, UIAdvertisingRegistrationPort);
	}
	
	public void Disconnect()
	{
		isRobotConnected = false;
		if (channel != null) {
			channel.Disconnect ();

			// only really needed in editor in case unhitting play
#if UNITY_EDITOR
			float limit = Time.realtimeSinceStartup + 2.0f;
			while (channel.HasPendingOperations) {
				if (limit < Time.realtimeSinceStartup) {
					Debug.LogWarning("Not waiting for disconnect to finish sending.", this);
					break;
				}
				System.Threading.Thread.Sleep (500);
			}
#endif
		}
	}

	public DisconnectionReason GetLastDisconnectionReason()
	{
		DisconnectionReason reason = lastDisconnectionReason;
		lastDisconnectionReason = DisconnectionReason.None;
		return reason;
	}
	 
	private void Connected(string connectionIdentifier)
	{
		if (ConnectedToClient != null) {
			ConnectedToClient(connectionIdentifier);
		}
	}
	 
	private void Disconnected(DisconnectionReason reason)
	{
		Debug.Log ("Disconnected: " + reason.ToString());
		isRobotConnected = false;
		Application.LoadLevel ("Shell");

		lastDisconnectionReason = reason;
		if (DisconnectedFromClient != null) {
			DisconnectedFromClient(reason);
		}
	}

	private void ReceivedMessage(G2U.Message message)
	{
		switch (message.GetTag ()) {
		case G2U.Message.Tag.Ping:
			break;
		case G2U.Message.Tag.RobotAvailable:
			ReceivedSpecificMessage(message.RobotAvailable);
			break;
		case G2U.Message.Tag.UiDeviceAvailable:
			ReceivedSpecificMessage(message.UiDeviceAvailable);
			break;
		case G2U.Message.Tag.RobotConnected:
			ReceivedSpecificMessage(message.RobotConnected);
			break;
		case G2U.Message.Tag.UiDeviceConnected:
			ReceivedSpecificMessage(message.UiDeviceConnected);
			break;
		case G2U.Message.Tag.RobotDisconnected:
			ReceivedSpecificMessage(message.RobotDisconnected);
			break;
		case G2U.Message.Tag.RobotObservedObject:
			ReceivedSpecificMessage(message.RobotObservedObject);
			break;
		case G2U.Message.Tag.RobotObservedNothing:
			ReceivedSpecificMessage(message.RobotObservedNothing);
			break;
		case G2U.Message.Tag.DeviceDetectedVisionMarker:
			ReceivedSpecificMessage(message.DeviceDetectedVisionMarker);
			break;
		case G2U.Message.Tag.PlaySound:
			ReceivedSpecificMessage(message.PlaySound);
			break;
		case G2U.Message.Tag.StopSound:
			ReceivedSpecificMessage(message.StopSound);
			break;
		case G2U.Message.Tag.ImageChunk:
			ReceivedSpecificMessage(message.ImageChunk);
			break;
		case G2U.Message.Tag.RobotState:
			ReceivedSpecificMessage(message.RobotState);
			break;
		case G2U.Message.Tag.RobotCompletedAction:
			ReceivedSpecificMessage(message.RobotCompletedAction);
			break;
		case G2U.Message.Tag.RobotDeletedObject:
			ReceivedSpecificMessage(message.RobotDeletedObject);
			break;
		default:
			Debug.LogWarning( message.GetTag() + " is not supported", this );
			break;
		}
	}
	
	private void ReceivedSpecificMessage(G2U.RobotAvailable message)
	{
		U2G.ConnectToRobot response = new U2G.ConnectToRobot();
		response.robotID = (byte)message.robotID;
		
		channel.Send (new U2G.Message{ConnectToRobot=response});
	}
	
	private void ReceivedSpecificMessage(G2U.UiDeviceAvailable message)
	{
		U2G.ConnectToUiDevice response = new U2G.ConnectToUiDevice ();
		response.deviceID = (byte)message.deviceID;
		
		channel.Send (new U2G.Message{ConnectToUiDevice=response});
	}
	
	private void ReceivedSpecificMessage(G2U.RobotConnected message)
	{
		// no longer a good indicator
//		if (RobotConnected != null) {
//			RobotConnected((int)message.robotID);
//		}
	}
	
	private void ReceivedSpecificMessage(G2U.UiDeviceConnected message)
	{
		Debug.Log ("Device connected: " + message.deviceID.ToString());
	}
	
	private void ReceivedSpecificMessage(G2U.RobotDisconnected message)
	{
		Debug.LogError ("Robot " + message.robotID + " disconnected after " + message.timeSinceLastMsg_sec.ToString ("0.00") + " seconds.", this);
		Disconnect ();
		Disconnected (DisconnectionReason.RobotDisconnected);
	}
	
	private void ReceivedSpecificMessage(G2U.RobotObservedObject message)
	{
		//Debug.Log( "box found with ID:" + message.objectID + " at " + Time.time );

		current.UpdateObservedObjectInfo( message );
	}

	private void ReceivedSpecificMessage( G2U.RobotObservedNothing message )
	{
		if( current.selectedObjects.Count == 0 && !current.isBusy )
		{
			//Debug.Log( "no box found" );

			current.ClearObservedObjects();
			//current.lastObjectHeadTracked = null;
		}
	}

	private void ReceivedSpecificMessage( G2U.RobotDeletedObject message )
	{
		Debug.Log( "Deleted object with ID " +message.objectID );

		ObservedObject deleted = current.knownObjects.Find( x=> x == message.objectID );

		if( deleted != null )
		{
			current.knownObjects.Remove( deleted );
		}

		deleted = current.selectedObjects.Find( x=> x == message.objectID );

		if( deleted != null )
		{
			current.selectedObjects.Remove( deleted );
		}

		deleted = current.observedObjects.Find( x=> x == message.objectID );
		
		if( deleted != null )
		{
			current.observedObjects.Remove( deleted );
		}

		deleted = current.markersVisibleObjects.Find( x=> x == message.objectID );
		
		if( deleted != null )
		{
			current.markersVisibleObjects.Remove( deleted );
		}
	}

	private void ReceivedSpecificMessage( G2U.RobotCompletedAction message )
	{
		bool success = (message.result == ActionResult.SUCCESS);
		RobotActionType action_type = (RobotActionType)message.actionType;
		Debug.Log("Action completed " + success);
		
		current.selectedObjects.Clear();
		current.targetLockedObject = null;
		
		current.SetHeadAngle();
		
		if(SuccessOrFailure != null) {
			SuccessOrFailure(success, action_type);
		}

		current.localBusyTimer = 0f;

		if(!success) {
			if(current.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) {
				current.SetLiftHeight(1f);
			}
			else {
				current.SetLiftHeight(0f);
			}
		}
	}

	private void ReceivedSpecificMessage(G2U.DeviceDetectedVisionMarker message)
	{

	}
	
	private void ReceivedSpecificMessage(G2U.PlaySound message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U.StopSound message)
	{
		
	}
	
	private void ReceivedSpecificMessage( G2U.RobotState message )
	{
		if (!isRobotConnected) {
			Debug.Log ("Robot " + message.robotID.ToString() + " sent first state message.");
			isRobotConnected = true;
			if (RobotConnected != null) {
			    RobotConnected(message.robotID);
			}
		}

		lastRobotStateMessage = Time.realtimeSinceStartup;

		if( !robots.ContainsKey( message.robotID ) )
		{
			Debug.Log( "adding robot with ID: " + message.robotID );
			
			AddRobot( message.robotID );
		}
		
		robots[ message.robotID ].UpdateInfo( message );
	}
	
	private Texture2D texture;
	private int currentImageIndex;
	private int currentChunkIndex;
	private UInt32 currentImageID = UInt32.MaxValue;
	private UInt32 currentImageFrameTimeStamp = UInt32.MaxValue;
	private Color32[] color32Array;
	private byte[] jpegArray;
	private byte[] minipegArray;
	private readonly byte[] header = new byte[]
	{
		0xFF, 0xD8, 0xFF, 0xE0, 0x00, 0x10, 0x4A, 0x46, 0x49, 0x46, 0x00, 0x01, 0x01, 0x00, 0x00, 0x01, 
		0x00, 0x01, 0x00, 0x00, 0xFF, 0xDB, 0x00, 0x43, 0x00, 0x10, 0x0B, 0x0C, 0x0E, 0x0C, 0x0A, 0x10, // 0x19 = QTable
		0x0E, 0x0D, 0x0E, 0x12, 0x11, 0x10, 0x13, 0x18, 0x28, 0x1A, 0x18, 0x16, 0x16, 0x18, 0x31, 0x23, 
		0x25, 0x1D, 0x28, 0x3A, 0x33, 0x3D, 0x3C, 0x39, 0x33, 0x38, 0x37, 0x40, 0x48, 0x5C, 0x4E, 0x40, 
		0x44, 0x57, 0x45, 0x37, 0x38, 0x50, 0x6D, 0x51, 0x57, 0x5F, 0x62, 0x67, 0x68, 0x67, 0x3E, 0x4D, 
		0x71, 0x79, 0x70, 0x64, 0x78, 0x5C, 0x65, 0x67, 0x63, 0xFF, 0xC0, 0x00, 0x0B, 0x08, 0x00, 0xF0, // 0x5E = Height x Width
		0x01, 0x40, 0x01, 0x01, 0x11, 0x00, 0xFF, 0xC4, 0x00, 0xD2, 0x00, 0x00, 0x01, 0x05, 0x01, 0x01, 
		0x01, 0x01, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x02, 0x03, 0x04, 
		0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x10, 0x00, 0x02, 0x01, 0x03, 0x03, 0x02, 0x04, 0x03, 
		0x05, 0x05, 0x04, 0x04, 0x00, 0x00, 0x01, 0x7D, 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 
		0x21, 0x31, 0x41, 0x06, 0x13, 0x51, 0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xA1, 0x08, 
		0x23, 0x42, 0xB1, 0xC1, 0x15, 0x52, 0xD1, 0xF0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0A, 0x16, 
		0x17, 0x18, 0x19, 0x1A, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 
		0x3A, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 
		0x5A, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6A, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79, 
		0x7A, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8A, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 
		0x99, 0x9A, 0xA2, 0xA3, 0xA4, 0xA5, 0xA6, 0xA7, 0xA8, 0xA9, 0xAA, 0xB2, 0xB3, 0xB4, 0xB5, 0xB6, 
		0xB7, 0xB8, 0xB9, 0xBA, 0xC2, 0xC3, 0xC4, 0xC5, 0xC6, 0xC7, 0xC8, 0xC9, 0xCA, 0xD2, 0xD3, 0xD4, 
		0xD5, 0xD6, 0xD7, 0xD8, 0xD9, 0xDA, 0xE1, 0xE2, 0xE3, 0xE4, 0xE5, 0xE6, 0xE7, 0xE8, 0xE9, 0xEA, 
		0xF1, 0xF2, 0xF3, 0xF4, 0xF5, 0xF6, 0xF7, 0xF8, 0xF9, 0xFA, 0xFF, 0xDA, 0x00, 0x08, 0x01, 0x01, 
		0x00, 0x00, 0x3F, 0x00
	};
 
	private void ReceivedSpecificMessage( G2U.ImageChunk message )
	{
		switch( (ImageEncoding_t)message.imageEncoding )
		{
			case ImageEncoding_t.IE_JPEG_COLOR:
				ColorJpeg( message );
				break;
			case ImageEncoding_t.IE_RAW_GRAY:
				GrayRaw( message );
				break;
			case ImageEncoding_t.IE_MINIPEG_GRAY:
				MinipegGray( message );
				break;
			default:
				Debug.LogWarning( (ImageEncoding_t)message.imageEncoding + " is not supported", this );
				break;
		}
	}

	private void MinipegGray( G2U.ImageChunk message )
	{
		if( minipegArray == null || message.imageId != currentImageID || message.frameTimeStamp != currentImageFrameTimeStamp )
		{
			currentImageID = message.imageId;
			currentImageFrameTimeStamp = message.frameTimeStamp;
			
			int length = message.chunkSize * message.imageChunkCount;
			
			if( minipegArray == null || minipegArray.Length < length )
			{
				minipegArray = new byte[ length ];
			}

			length = minipegArray.Length * 2 + header.Length;
			
			if( jpegArray == null || jpegArray.Length < length )
			{
				// Allocate enough space for worst case expansion
				jpegArray = new byte[ length ];
				Array.Copy( header, jpegArray, header.Length );
			}

			currentImageIndex = 0;
			currentChunkIndex = 0;
		}
		
		for( int messageIndex = 0; currentImageIndex < minipegArray.Length && messageIndex < message.chunkSize; ++messageIndex, ++currentImageIndex )
		{
			minipegArray[ currentImageIndex ] = message.data[ messageIndex ];
		}
		
		if( ++currentChunkIndex == message.imageChunkCount )
		{
			int width = message.ncols;
			int height = message.nrows;
			
			if( texture == null || texture.width != width || texture.height != height )
			{
				if( texture != null )
				{
					Destroy( texture );
					texture = null;
				}
				
				texture = new Texture2D( width, height, TextureFormat.RGB24, false );
			}

			MiniGrayToJpeg( minipegArray, ref jpegArray );

			texture.LoadImage( jpegArray );
			
			if( RobotImage != null )
			{
				RobotImage( texture );
				
				current.ClearObservedObjects();
			}
			
			SaveJpeg( jpegArray, currentImageIndex );
		}
	}

	private void MiniGrayToJpeg( byte[] inArray, ref byte[] outArray )
	{
		int off = header.Length;
		
		// Fetch quality
		int qual = inArray[0];
		
		// Add byte stuffing - one 0 after each 0xff
		for( int i = 1; i < inArray.Length; ++i )
		{
			outArray[off++] = inArray[i];
			if( inArray[i] == 0xff ) outArray[off++] = 0;
		}
		
		outArray[off++] = 0xFF;
		outArray[off++] = 0xD9;
	}

	private void GrayRaw( G2U.ImageChunk message )
	{
		if( color32Array == null || message.imageId != currentImageID || message.frameTimeStamp != currentImageFrameTimeStamp )
		{
			currentImageID = message.imageId;
			currentImageFrameTimeStamp = message.frameTimeStamp;
			
			int length = message.ncols * message.nrows;
			
			if( color32Array == null || color32Array.Length < length )
			{
				color32Array = new Color32[ length ];
			}
			
			currentImageIndex = 0;
		}
		
		for( int messageIndex = 0; currentImageIndex < color32Array.Length && messageIndex < message.chunkSize; ++messageIndex, ++currentImageIndex )
		{
			byte gray = message.data[ messageIndex ];
			
			int x = currentImageIndex % message.ncols;
			int y = currentImageIndex / message.ncols;
			int index = message.ncols * ( message.nrows - y - 1 ) + x;
			
			color32Array[ index ] = new Color32( gray, gray, gray, 255 );
		}
		
		if( currentImageIndex == color32Array.Length )
		{
			int width = message.ncols;
			int height = message.nrows;
			
			if( texture == null || texture.width != width || texture.height != height )
			{
				if( texture != null )
				{
					Destroy( texture );
					texture = null;
				}
				
				texture = new Texture2D( width, height, TextureFormat.ARGB32, false );
			}
			
			texture.SetPixels32( color32Array );
			texture.Apply( false );
			
			if( RobotImage != null )
			{
				RobotImage( texture );
				
				current.ClearObservedObjects();
			}
		}
	}

	private void ColorJpeg( G2U.ImageChunk message )
	{
		if( jpegArray == null || message.imageId != currentImageID || message.frameTimeStamp != currentImageFrameTimeStamp )
		{
			currentImageID = message.imageId;
			currentImageFrameTimeStamp = message.frameTimeStamp;

			int length = message.chunkSize * message.imageChunkCount;
			
			if( jpegArray == null || jpegArray.Length < length )
			{
				jpegArray = new byte[ length ];
			}
			
			currentImageIndex = 0;
			currentChunkIndex = 0;
		}
		
		for( int messageIndex = 0; currentImageIndex < jpegArray.Length && messageIndex < message.chunkSize; ++messageIndex, ++currentImageIndex )
		{
			jpegArray[ currentImageIndex ] = message.data[ messageIndex ];
		}

		if( ++currentChunkIndex == message.imageChunkCount )
		{
			int width = message.ncols;
			int height = message.nrows;
			
			if( texture == null || texture.width != width || texture.height != height )
			{
				if( texture != null )
				{
					Destroy( texture );
					texture = null;
				}
				
				texture = new Texture2D( width, height, TextureFormat.RGB24, false );
			}

			texture.LoadImage( jpegArray );
			
			if( RobotImage != null )
			{
				RobotImage( texture );
				
				current.ClearObservedObjects();
			}

			SaveJpeg( jpegArray, currentImageIndex );
		}
	}

	public void SaveJpeg(byte[] buffer, int length)
	{
		imageFrameCount++;
		if (!AllowImageSaving || !imageDirectoryCreated) {
			return;
		}
//		if (imageFrameSkip != 0) {
//			if (imageFrameCount % imageFrameSkip != 0) {
//				return;
//			}
//		}
		string filename = string.Format("frame-{0}_time-{1}.jpg", imageFrameCount, Time.time);
		string filepath = Path.Combine (imageBasePath, filename);
		SaveAsync(filepath, buffer, 0, length);
	}


	public void SaveAsync(string filepath, byte[] buffer, int start, int length)
	{
		FileStream stream = new FileStream(filepath, FileMode.Create, FileAccess.Write, FileShare.None, 0x1000, true);
		try {
			stream.BeginWrite(buffer, start, length, EndSave_callback, stream);
		}
		catch (Exception e) {
			Debug.LogException(e, this);
		}
	}

	private void EndSave(IAsyncResult result)
	{
		lock (sync) {
			try {
				FileStream stream = (FileStream)result.AsyncState;
				stream.EndWrite(result);
			}
			catch (Exception e) {
				safeLogs.Add(e);
			}
		}
	}

	public void VisualizeQuad( uint ID, uint color, Vector3 upperLeft, Vector3 upperRight, Vector3 lowerRight, Vector3 lowerLeft )
	{
		U2G.VisualizeQuad message = new U2G.VisualizeQuad();

		message.color = color;
		message.quadID = ID;

		message.xUpperLeft = upperLeft.x;
		message.xUpperRight = upperRight.x;
		message.xLowerLeft = lowerLeft.x;
		message.xLowerRight = lowerRight.x;

		message.yUpperLeft = upperLeft.y;
		message.yUpperRight = upperRight.y;
		message.yLowerLeft = lowerLeft.y;
		message.yLowerRight = lowerRight.y;

		message.zUpperLeft = upperLeft.z;
		message.zUpperRight = upperRight.z;
		message.zLowerLeft = lowerLeft.z;
		message.zLowerRight = lowerRight.z;

		RobotEngineManager.instance.channel.Send( new U2G.Message{ VisualizeQuad = message } );
	}

	public void StartEngine(string vizHostIP)
	{
		U2G.StartEngine message = new U2G.StartEngine ();
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
		
		channel.Send (new U2G.Message{StartEngine=message});
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
		
		U2G.ForceAddRobot message = new U2G.ForceAddRobot ();
		if (Encoding.UTF8.GetByteCount (robotIP) + 1 > message.ipAddress.Length) {
			throw new ArgumentException("IP address too long.", "robotIP");
		}
		int length = Encoding.UTF8.GetBytes (robotIP, 0, robotIP.Length, message.ipAddress, 0);
		message.ipAddress [length] = 0;
		
		message.robotID = (byte)robotID;
		message.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;
		
		channel.Send (new U2G.Message{ForceAddRobot=message});
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

	public enum ImageEncoding_t
	{
		IE_NONE,
		IE_RAW_GRAY, // no compression
		IE_RAW_RGB,  // no compression, just [RGBRGBRG...]
		IE_YUYV,
		IE_BAYER,
		IE_JPEG_GRAY,
		IE_JPEG_COLOR,
		IE_JPEG_CHW, // Color half width
		IE_MINIPEG_GRAY   // Minimized grayscale JPEG - no header, no footer, no byte stuffing
	}
}
