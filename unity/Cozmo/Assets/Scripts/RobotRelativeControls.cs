using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

#region INSPECTOR FIELDS
	//[SerializeField] ScreenRecorder recorder = null;
	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] GyroControls gyroInputs = null;
	[SerializeField] float gyroSleepTime = 3f;
	//[SerializeField] AccelControls accelInputs = null;
	[SerializeField] Toggle gyroRollControl = null;
	[SerializeField] Toggle gyroPitchControl = null;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;
	[SerializeField] float swipeTurnAngle = 0f;
	[SerializeField] bool doubleTapTurnAround = true;

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
	bool aboutFace = false;
	float gyroSleepTimer = 0f;
	float maxTurnFactor = 1f;
	bool reverseLikeACar = true;
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
		aboutFace = false;

		maxTurnFactor = PlayerPrefs.GetFloat("MaxTurnFactor", OptionsScreen.DEFAULT_MAX_TURN_FACTOR);
		reverseLikeACar = PlayerPrefs.GetInt("ReverseLikeACar", OptionsScreen.REVERSE_LIKE_A_CAR) == 1;
		//Debug.Log(gameObject.name + " OnEnable reverseLikeACar("+reverseLikeACar+")");

	}

	void Update() {


		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			robotFacing = MathUtil.ClampAngle(RobotEngineManager.instance.current.poseAngle_rad * Mathf.Rad2Deg);

			//if coz is picked up, let's zero our wheels and abort control logic
			if(RobotEngineManager.instance.current.status == Robot.StatusFlag.IS_PICKED_UP) {
				if(Intro.CurrentRobotID != 0) {
					RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, 0f, 0f);
					return;
				}
			}

		}

		if(aboutFace) {
			float turnSoFar = MathUtil.AngleDelta(robotStartTurnFacing, robotFacing);
			if(Mathf.Abs(turnSoFar) > 180f) {
				//Debug.Log("frame(" + Time.frameCount + ") EndSwipe turnSoFar(" + turnSoFar + ")");
				aboutFace = false;
				if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
					RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, 0f, 0f);
				}
			}
			return;
		}

		if(debugOverride) {
			if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
				RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
			}
			RefreshDebugText();
			return;
		}

		timeSinceLastCommand += Time.deltaTime;
		if(timeSinceLastCommand < refreshTime) return;

		timeSinceLastCommand = 0f;

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		bool driveForwardOnlyMode = false;
		bool driveReverseOnlyMode = false;
		bool turnInPlaceOnlyMode = false;

		float maxAngle = 90f;

		if(verticalStick != null) {

			if(doubleTapTurnAround && verticalStick.DoubleTapped) {
				if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
					aboutFace = true;
					robotStartTurnFacing = robotFacing;

					verticalStick.AbsorbDoubleTap();
					RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, CozmoUtil.MAX_WHEEL_SPEED, -CozmoUtil.MAX_WHEEL_SPEED);
					return;
				}
			}

			if(verticalStick.UpModeEngaged) {
				driveForwardOnlyMode = true;
				maxAngle = verticalStick.MaxAngle;
			}
			else if(verticalStick.DownModeEngaged) {
				driveReverseOnlyMode = true;
				maxAngle = verticalStick.MaxAngle;
			}

			inputs.y = verticalStick.Vertical;
		}

		if(!swipeTurning) swipeTurnIndex = 0;

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
					swipeTurning = true;
					//Debug.Log("frame("+Time.frameCount+") swipeTurning begin swipeTurnIndex("+swipeTurnIndex+")");
					horizontalStick.AbsorbSwipeRequest();
				}
			}

			if(!horizontalStick.SwipeActive) {
				swipeTurning = false;
				swipeTurnIndex = 0;
			}

			if(swipeTurning) {

				float turnSoFar = MathUtil.AngleDelta(robotStartTurnFacing, robotFacing);

				float goalAngle = swipeTurnAngle;

				for(int i=0;i<swipeTurnIndex && i<3;i++) {
					goalAngle *= 2f;
				}

				if(!horizontalStick.SwipeActive || Mathf.Abs(turnSoFar) > goalAngle) {
					//Debug.Log("frame("+Time.frameCount+") EndSwipe turnSoFar(" + turnSoFar + ") goalAngle(" + goalAngle + ")");
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

		gyroSleepTimer += Time.deltaTime;
		if(verticalStick == null || verticalStick.IsPressed) {
			gyroSleepTimer = 0f;
		}
		else if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			switch(RobotEngineManager.instance.current.status) {
				case Robot.StatusFlag.IS_ANIMATING:
				case Robot.StatusFlag.IS_PICKING_OR_PLACING:
					gyroSleepTimer = 0f;
					break;
			}
		}
		
		
		if(gyroSleepTimer <= gyroSleepTime && gyroInputs != null && gyroInputs.gameObject.activeSelf) { // && (verticalStick == null || verticalStick.IsPressed)) {

			float h = gyroInputs.Horizontal;
			inputs.x += h;
			inputs.x = Mathf.Clamp(inputs.x, -1f, 1f);

//			if(gyroPitchControl != null && gyroPitchControl.isOn) {
//				inputs.y = gyroInputs.Vertical;
//			}

			if(gyroInputs.Horizontal != 0f) gyroSleepTimer = 0f; 
		}

		if(inputs.x == 0f && inputs.y == 0f) {
			inputs.x = Input.GetAxis("Horizontal");
			inputs.y = Input.GetAxis("Vertical");
		}

		bool stopped = inputs.sqrMagnitude == 0f && moveCommandLastFrame;
		if(!stopped) {
			Vector3 delta = inputs - lastInputs;
			if(delta.sqrMagnitude <= 0f)
				return; // command not changed
		}

		lastInputs = inputs;

		if(!reverseLikeACar && !driveForwardOnlyMode && (inputs.y < 0f || driveReverseOnlyMode)) {
			inputs.x = -inputs.x;
		}

		if(driveForwardOnlyMode) {
			CozmoUtil.CalcWheelSpeedsForThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, maxTurnFactor, false);
		}
		else if(driveReverseOnlyMode) {
			CozmoUtil.CalcWheelSpeedsForThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, maxTurnFactor, true);
		}
		else if(turnInPlaceOnlyMode || inputs.y == 0f) {
			CozmoUtil.CalcTurnInPlaceWheelSpeeds(inputs.x, out leftWheelSpeed, out rightWheelSpeed, maxTurnFactor);
		}
		else if(verticalStick == horizontalStick) {
			CozmoUtil.CalcWheelSpeedsForOldThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, maxTurnFactor);
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

	public void ReverseButtonPressed(bool pressed) {
		if(verticalStick == null)
			return;

		verticalStick.SetForceReverse(pressed);

	}

#endregion

}
