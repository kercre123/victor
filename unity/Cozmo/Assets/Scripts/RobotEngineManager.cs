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

#if UNITY_IOS && !UNITY_EDITOR
	private StringBuilder logBuilder = null;
#endif

	void Awake() {
		if (instance != null) {
			Destroy (gameObject);
		} else {
			instance = this;
			DontDestroyOnLoad (gameObject);
		}
	}

#if UNITY_IOS && !UNITY_EDITOR

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
		
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_enginehost_create (configuration.text);
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_enginehost_create error: " + result.ToString());
		} else {
			engineHostInitialized = true;
		}
	}
	
	void OnDestroy()
	{
		if (engineHostInitialized) {
			engineHostInitialized = false;
			
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_enginehost_destroy();
			if (result != CozmoResult.OK) {
				Debug.LogError("cozmo_enginehost_forceaddrobot error: " + result.ToString());
			}
		}
	}
	
	void Update()
	{
		if (logBuilder == null) {
			logBuilder = new StringBuilder (1024);
		}
		
		int length = 0;
		while (CozmoBinding.cozmo_has_log(ref length)) {
			if (logBuilder.Capacity < length) {
				logBuilder.Capacity = Math.Max (logBuilder.Capacity * 2, length);
			}
			
			CozmoBinding.cozmo_pop_log (logBuilder, logBuilder.Capacity);
			Debug.LogError (logBuilder.ToString ());
		}
		
		if (engineHostInitialized) {
			CozmoResult result = (CozmoResult)CozmoBinding.cozmo_enginehost_update (Time.realtimeSinceStartup);
			if (result != CozmoResult.OK) {
				Debug.LogError ("cozmo_enginehost_update error: " + result.ToString ());
			}
		}
	}
#endif

	/// <summary>
	/// Forcibly adds a new robot.
	/// </summary>
	/// <param name="robotId">The robot identifier.</param>
	/// <param name="robotIP">The ip address the robot is connected to.</param>
	/// <param name="robotIsSimulated">Specify true for a simulated robot.</param>
	public CozmoResult ForceAddRobot(int robotId, string robotIP, bool robotIsSimulated)
	{
		if (!engineHostInitialized) {
			Debug.LogError("Error forcibly adding robot: Cannot add a robot when the host is not initialized.");
			return CozmoResult.BINDING_ERROR_NOT_INITIALIZED;
		}

#if UNITY_IOS && !UNITY_EDITOR
		CozmoResult result = (CozmoResult)CozmoBinding.cozmo_enginehost_forceaddrobot (robotId, robotIP, robotIsSimulated);
		if (result != CozmoResult.OK) {
			Debug.LogError("cozmo_enginehost_forceaddrobot error: " + result.ToString());
		}
		return result;
#else
		return CozmoResult.CSHARP_NOT_AVAILABLE;
#endif
	}

}
