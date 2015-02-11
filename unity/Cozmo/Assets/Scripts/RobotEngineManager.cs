
using UnityEngine;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Text;

public class RobotEngineManager : MonoBehaviour {
	
	public static RobotEngineManager instance = null;
	
	[SerializeField]
	private TextAsset configuration;
	
	private bool engineHostInitialized = false;
	
	public bool IsHostInitialized {
		get {
			return engineHostInitialized;
		}
	}

	public event Action<string> ConnectedToClient;
	public event Action<DisconnectionReason> DisconnectedFromClient;

	private ChannelBase channel;

#if !UNITY_EDITOR
	private StringBuilder logBuilder = null;
#endif
	
	void Awake() {
		if (instance != null) {
			Destroy (gameObject);
		} else {
			instance = this;
			DontDestroyOnLoad (gameObject);
		}

		channel = new UdpChannel ();
		channel.ConnectedToClient += Connected;
		channel.DisconnectedFromClient += Disconnected;
		channel.MessageReceived += ReceivedMessage;
	}

#if !UNITY_EDITOR
	void Start()
	{
		if (engineHostInitialized) {
			Debug.LogError("Error initializing cozmo host: Host already started.");
			return;
		}
		
		if (configuration == null) {
			Debug.LogError("Error initializing cozmo host: No configuration.");
			return;
		}
		
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_engine_host_create (configuration.text);
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_engine_host_create error: " + result.ToString());
		} else {
			engineHostInitialized = true;
		}
	}
	
	void OnDestroy()
	{
		if (engineHostInitialized) {
			engineHostInitialized = false;
			
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_engine_host_destroy();
			if (result != CozmoResult.OK) {
				Debug.LogError("cozmo_engine_host_destroy error: " + result.ToString());
			}
		}
	}
	
	void Update()
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
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_engine_update (Time.realtimeSinceStartup);
			if (result != CozmoResult.OK) {
				Debug.LogError ("cozmo_engine_update error: " + result.ToString ());
			}
		}
	}
#endif

	public void Connect(string engineIP)
	{
		int deviceID = 1;
		int enginePort = 5000;
		int localPort = 5002;

		channel.Connect (deviceID, localPort, engineIP, enginePort);
	}

	public void Disconnect()
	{
		channel.Disconnect ();
	}

	private void Connected(string connectionIdentifier)
	{
		if (ConnectedToClient != null) {
			ConnectedToClient(connectionIdentifier);
		}
	}

	private void Disconnected(DisconnectionReason reason)
	{
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
		case (int)NetworkMessageID.G2U_RobotObservedObject:
			ReceivedSpecificMessage((G2U_RobotObservedObject)message);
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
		}
	}
	
	private void ReceivedSpecificMessage(G2U_RobotAvailable message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_UiDeviceAvailable message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_RobotConnected message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_UiDeviceConnected message)
	{
		
	}
	
	private void ReceivedSpecificMessage(G2U_RobotObservedObject message)
	{
		
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
	
	/// <summary>
	/// Forcibly adds a new robot.
	/// </summary>
	/// <param name="robotId">The robot identifier.</param>
	/// <param name="robotIP">The ip address the robot is connected to.</param>
	/// <param name="robotIsSimulated">Specify true for a simulated robot.</param>
	public CozmoResult ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated)
	{


#if UNITY_IOS && !UNITY_EDITOR
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_engine_host_force_add_robot (robotID, robotIP, robotIsSimulated);
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_engine_host_force_add_robot error: " + result.ToString());
		}
		return result;
#else
		return CozmoResult.CSHARP_NOT_AVAILABLE;
#endif
	}
	
	public bool IsRobotConnected(int robotID)
	{
#if UNITY_IOS && !UNITY_EDITOR
		Update();
		
		bool isConnected;
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_engine_host_is_robot_connected (out isConnected, robotID);
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_engine_get_number_of_robots error: " + result.ToString());
			return false;
		}
		return isConnected;
#else
		return false;
#endif
		
	}
	
	/// <summary>
	/// Set wheel speed.
	/// </summary>
	/// <param name="left_wheel_speed_mmps">Left wheel speed in millimeters per second.</param>
	/// <param name="right_wheel_speed_mmps">Right wheel speed in millimeters per second.</param>
	public void DriveWheels(int robotID, float leftWheelSpeedMmps, float rightWheelSpeedMmps)
	{
#if UNITY_IOS && !UNITY_EDITOR
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_robot_drive_wheels (robotID, leftWheelSpeedMmps, rightWheelSpeedMmps);
		if (result != CozmoResult.OK) {
			Debug.LogError ("cozmo_robot_drive_wheels error: " + result.ToString());
		}
#endif
	}
	
	public void StopAllMotors(int robotID)
	{
		#if UNITY_IOS && !UNITY_EDITOR
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_robot_stop_all_motors (robotID);
		if (result != CozmoResult.OK) {
			Debug.LogError ("cozmo_robot_stop_all_motors error: " + result.ToString());
		}
		#endif
	}
	
	public const float MAX_WHEEL_SPEED 	= 100f;
	public const float MAX_ANALOG_RADIUS   = 300f;
	public const float HALF_PI             = Mathf.PI * 0.5f;
	public const float wheelDistHalfMM 	= 23.85f; //47.7f / 2.0; // distance b/w the front wheels
	
	public static void CalcWheelSpeedsFromBotRelativeInputs(Vector2 inputs, out float leftWheelSpeed, out float rightWheelSpeed) {
		
		leftWheelSpeed = 0f;
		rightWheelSpeed = 0f;
		
		if(inputs.x == 0f && inputs.y == 0f)
			return;
		
		// Compute speed
		float xyMag = Mathf.Min(1f, inputs.magnitude);
		
		// Driving forward?
		float fwd = inputs.y > 0f ? 1f : -1f;
		
		// Curving right?
		float right = inputs.x > 0f ? 1f : -1f;
		
		// Base wheel speed based on magnitude of input and whether or not robot is driving forward
		float baseWheelSpeed = MAX_WHEEL_SPEED * xyMag * fwd;
		
		// Angle of xy input used to determine curvature
		float xyAngle = 0f;
		if(inputs.x != 0f) xyAngle = Mathf.Abs(Mathf.Atan((float)inputs.y / (float)inputs.x)) * right;
		
		// Compute radius of curvature
		float roc = (xyAngle / HALF_PI) * MAX_ANALOG_RADIUS;
		
		// Compute individual wheel speeds
		if(inputs.x == 0f) { //Mathf.Abs(xyAngle) > HALF_PI - 0.1f) {
			// Straight fwd/back
			leftWheelSpeed = baseWheelSpeed;
			rightWheelSpeed = baseWheelSpeed;
		}
		else if(inputs.y == 0f) { //Mathf.Abs(xyAngle) < 0.1f) {
			// Turn in place
			leftWheelSpeed = right * xyMag * MAX_WHEEL_SPEED;
			rightWheelSpeed = -right * xyMag * MAX_WHEEL_SPEED;
		}
		else {
			
			leftWheelSpeed = baseWheelSpeed * (roc + (right * wheelDistHalfMM)) / roc;
			rightWheelSpeed = baseWheelSpeed * (roc - (right * wheelDistHalfMM)) / roc;
			
			// Swap wheel speeds
			if(fwd * right < 0f) {
				float temp = leftWheelSpeed;
				leftWheelSpeed = rightWheelSpeed;
				rightWheelSpeed = temp;
			}
		}
		
		
		// Cap wheel speed at max
		if(Mathf.Abs(leftWheelSpeed) > MAX_WHEEL_SPEED) {
			float correction = leftWheelSpeed - (MAX_WHEEL_SPEED * (leftWheelSpeed >= 0f ? 1f : -1f));
			float correctionFactor = 1f - Mathf.Abs(correction / leftWheelSpeed);
			leftWheelSpeed *= correctionFactor;
			rightWheelSpeed *= correctionFactor;
			//printf("lcorrectionFactor: %f\n", correctionFactor);
		}
		
		if(Mathf.Abs(rightWheelSpeed) > MAX_WHEEL_SPEED) {
			float correction = rightWheelSpeed - (MAX_WHEEL_SPEED * (rightWheelSpeed >= 0f ? 1f : -1f));
			float correctionFactor = 1f - Mathf.Abs(correction / rightWheelSpeed);
			leftWheelSpeed *= correctionFactor;
			rightWheelSpeed *= correctionFactor;
			//printf("rcorrectionFactor: %f\n", correctionFactor);
		}
		
	}
	
}
