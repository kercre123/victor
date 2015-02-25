using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

#region INSPECTOR FIELDS
	//[SerializeField] ScreenRecorder recorder = null;
	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] GyroControls gyroInputs = null;
	//[SerializeField] AccelControls accelInputs = null;
	[SerializeField] Toggle gyroRollControl = null;
	[SerializeField] Toggle gyroPitchControl = null;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;
	[SerializeField] float swipeTurnAngle = 0f;
#endregion

#region MISC MEMBERS
	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0f;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;
	//bool reverseLikeACar = false;
	bool moveCommandLastFrame = false;
	bool debugOverride = false;
	float robotFacing = 0f;
	float robotStartTurnFacing = 0f;
	int swipeTurnIndex = 0;
	bool swipeTurning = false;
#endregion

#region COMPONENT CALLBACKS
	void OnEnable() {
		//reset default state for this control scheme test
		Debug.Log("RobotRelativeControls OnEnable");
		if(gyroRollControl != null) {
			gyroRollControl.isOn = true;
			//gyroRollControl.gameObject.SetActive(gyroInputs != null);
		}

		if(gyroPitchControl != null) {
			gyroPitchControl.isOn = false;
			//gyroPitchControl.gameObject.SetActive(gyroInputs != null);
		}

		lastInputs = Vector2.zero;
		moveCommandLastFrame = false;
		timeSinceLastCommand = 0f;
		debugOverride = false;
		swipeTurning = false;
//		if(recorder != null) {
//			recorder.videoFileName = "cozmoTest_screenRec_" + System.DateTime.UtcNow.ToString("yyyy-MM-dd_HH-mm-ss") + "_" + gameObject.name + ".mp4";
//			recorder.enabled = true;
//		}
	}

	void FixedUpdate() {

		bool robotFacingStale = true;
		if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
			Robot robot;
			if(RobotEngineManager.instance.robots.TryGetValue(Intro.CurrentRobotID, out robot)) {
				robotFacing = robot.poseAngle_rad * Mathf.Rad2Deg;
				robotFacingStale = false;
				//Debug.Log("frame("+Time.frameCount+") robotFacing(" + robotFacing + ")");
			}
		}

		if(debugOverride) {
			if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
				RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
			}
			RefreshDebugText();
			return;
		}
		timeSinceLastCommand += Time.deltaTime;

		if(timeSinceLastCommand < refreshTime)
			return;

		timeSinceLastCommand = 0f;

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		bool driveForwardOnlyMode = false;
		bool driveReverseOnlyMode = false;
		bool turnInPlaceOnlyMode = false;

		if(!swipeTurning)
			swipeTurnIndex = 0;

		if(horizontalStick != null) {
			if(horizontalStick.SideModeEngaged) {
				turnInPlaceOnlyMode = true;
			}

			if(swipeTurnAngle > 0f && horizontalStick.SwipeRequest && horizontalStick.SwipeActive) {

				float sideSwipeDot = Vector2.Dot(horizontalStick.SwipedDirection.normalized, Vector2.right);
				if(Mathf.Abs(sideSwipeDot) > 0.9f) {
					if(!swipeTurning) {
						robotStartTurnFacing = robotFacing;
					}
					else {
						swipeTurnIndex++;
					}
//					if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
//						RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, 0f, 0f);
//						RobotEngineManager.instance.TurnInPlace(Intro.CurrentRobotID, -swipeTurnAngle * Mathf.Deg2Rad);
//					}

					swipeTurning = true;
					Debug.Log("frame("+Time.frameCount+") swipeTurning begin swipeTurnIndex("+swipeTurnIndex+")");
					horizontalStick.AbsorbSwipeRequest();

					//return;
				}
			}

			if(!horizontalStick.SwipeActive) {
				swipeTurning = false;
				swipeTurnIndex = 0;
			}

			if(swipeTurning) {
				float turnSoFar = robotFacing - robotStartTurnFacing;

				while(turnSoFar > 180f) turnSoFar -= 360f;
				while(turnSoFar < -180f) turnSoFar += 360f;

				float goalAngle = swipeTurnAngle;

				for(int i=0;i<swipeTurnIndex && i<3;i++) {
					goalAngle *= 2f;
				}

				if(!horizontalStick.SwipeActive || Mathf.Abs(turnSoFar) > goalAngle) {
					Debug.Log("frame("+Time.frameCount+") EndSwipe turnSoFar(" + turnSoFar + ") goalAngle(" + goalAngle + ")");
					horizontalStick.EndSwipe();
					swipeTurning = false;
				}
				else if(!horizontalStick.IsPressed) {
					//Debug.Log("frame("+Time.frameCount+") swipeTurning turnSoFar(" + turnSoFar + ") goalAngle(" + goalAngle + ") input.x("+horizontalStick.Horizontal+")");
					inputs.x = horizontalStick.Horizontal;
				}
			}
			//for now lets ignore stick hor while swipe input is still underway
			else {
				inputs.x = horizontalStick.Horizontal;
				//Debug.Log("frame("+Time.frameCount+") input.x("+horizontalStick.Horizontal+")");
			}
		}
		else {
			swipeTurning = false;
		}

		float maxAngle = 90f;

		if(verticalStick != null) {

			if(verticalStick.UpModeEngaged) {
				driveForwardOnlyMode = true;
				//maxAngle = verticalStick.MaxAngle;
			}
			else if(verticalStick.DownModeEngaged) {
				driveReverseOnlyMode = true;
				//maxAngle = verticalStick.MaxAngle;
			}

			inputs.y = verticalStick.Vertical;
		}

//		if(inputs.x == 0f && inputs.y == 0f) {
//			inputs.x = Input.GetAxis("Horizontal");
//			inputs.y = Input.GetAxis("Vertical");
//		}

		if(gyroInputs != null && gyroInputs.gameObject.activeSelf) { // && (verticalStick == null || verticalStick.IsPressed)) {
			inputs.x = gyroInputs.Horizontal;
//			if(gyroPitchControl != null && gyroPitchControl.isOn) {
//				inputs.y = gyroInputs.Vertical;
//			}
		}

		//if(accelInputs != null && accelInputs.gameObject.activeSelf && (verticalStick == null || verticalStick.IsPressed) ) {
		//	inputs.x = accelInputs.Horizontal;
		//}

		bool stopped = inputs.sqrMagnitude == 0f && moveCommandLastFrame;
		if(!stopped) {
			Vector3 delta = inputs - lastInputs;
			if(delta.sqrMagnitude <= 0f)
				return; // command not changed
		}

		lastInputs = inputs;

		float maxTurnFactor = PlayerPrefs.GetFloat("MaxTurnFactor", OptionsScreen.DEFAULT_MAX_TURN_FACTOR);

		if(driveForwardOnlyMode) {
			CozmoUtil.CalcWheelSpeedsForThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, maxTurnFactor, false);
		}
		else if(driveReverseOnlyMode) {
			CozmoUtil.CalcWheelSpeedsForThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, maxTurnFactor, true);
		}
		else if(turnInPlaceOnlyMode || inputs.y == 0f) {
			CozmoUtil.CalcTurnInPlaceWheelSpeeds(inputs.x, out leftWheelSpeed, out rightWheelSpeed, maxTurnFactor);
		}
		else { //continuous input range mode...causes issues at thresholds
			CozmoUtil.CalcWheelSpeedsForTwoAxisInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxTurnFactor);
		}

		if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
		}

		moveCommandLastFrame = inputs.sqrMagnitude > 0f;

		RefreshDebugText();
	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("RobotRelativeControls OnDisable");

		if(RobotEngineManager.instance != null && RobotEngineManager.instance.IsConnected) {
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, 0f, 0f);
		}

//		if(recorder != null) {
//			recorder.enabled = false;
//		}
	}

//	void OnGUI() {
//		GUILayout.BeginArea(new Rect(Screen.width*0.5f-150f, 300f, 300f, 300f));
//		GUILayout.Label("input("+inputs+")");
//		GUILayout.Label("leftWheelSpeed("+leftWheelSpeed+")");
//		GUILayout.Label("rightWheelSpeed("+rightWheelSpeed+")");
//		GUILayout.EndArea();
//	}
#endregion

#region PRIVATE METHODS
	void RefreshDebugText() {
		if(text_x != null) text_x.text = "x(" + inputs.x.ToString("N") + ")";
		if(text_y != null) text_y.text = "y(" + inputs.y.ToString("N") + ")";
		
		if(text_leftWheelSpeed != null) text_leftWheelSpeed.text = "L(" + leftWheelSpeed.ToString("N") + ")";
		if(text_rightWheelSpeed != null) text_rightWheelSpeed.text = "R(" + rightWheelSpeed.ToString("N") + ")";
	}
#endregion

#region PUBLIC METHODS
	public void ToggleRollControl() {
		if(gyroRollControl != null) gyroRollControl.isOn = !gyroRollControl.isOn;
	}

	public void TogglePitchControl() {
		if(gyroPitchControl != null) gyroPitchControl.isOn = !gyroPitchControl.isOn;
	}

	public void SetReverseLikeACar(bool on) {
		Debug.Log(gameObject.name + " RobotRelativeControls SetReverseLikeACar("+on+")");
		//reverseLikeACar = on;
	}

	public void NudgeForward() {
		if(!debugOverride) {
			leftWheelSpeed = 0f;
			rightWheelSpeed = 0f;
		}
		leftWheelSpeed += 1f;
		rightWheelSpeed += 1f;
		debugOverride = true;
		Debug.Log(gameObject.name + " NudgeForward");
	}

	public void NudgeBackwards() {
		if(!debugOverride) {
			leftWheelSpeed = 0f;
			rightWheelSpeed = 0f;
		}
		leftWheelSpeed -= 1f;
		rightWheelSpeed -= 1f;
		debugOverride = true;
		Debug.Log(gameObject.name + " NudgeBackwards");
	}

	public void NudgeRight() {
		if(!debugOverride) {
			leftWheelSpeed = 0f;
			rightWheelSpeed = 0f;
		}
		leftWheelSpeed += 1f;
		rightWheelSpeed -= 1f;
		debugOverride = true;
		Debug.Log(gameObject.name + " NudgeRight");
	}

	public void NudgeLeft() {
		if(!debugOverride) {
			leftWheelSpeed = 0f;
			rightWheelSpeed = 0f;
		}
		leftWheelSpeed -= 1f;
		rightWheelSpeed += 1f;
		debugOverride = true;
		Debug.Log(gameObject.name + " NudgeRight");
	}
#endregion

}
