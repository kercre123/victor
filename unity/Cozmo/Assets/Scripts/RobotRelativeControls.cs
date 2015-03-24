using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

#region INSPECTOR FIELDS
	//[SerializeField] ScreenRecorder recorder = null;
	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] VirtualStick headAngleStick = null;
	[SerializeField] GyroControls gyroInputs = null;
	[SerializeField] float gyroSleepTime = 3f;
	//[SerializeField] AccelControls accelInputs = null;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;
	[SerializeField] float swipeTurnAngle = 0f;
	[SerializeField] bool doubleTapTurnAround = true;
	[SerializeField] float headAngleChangeRate = 1f;
	[SerializeField] float targetLockMaxTurnAngle = 25f;
	[SerializeField] float targetLockMinTurnAngle = 4f;

#endregion

#region MISC MEMBERS
	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;
	bool moveCommandLastFrame = false;
	bool headCommandLastFrame = false;
	bool debugOverride = false;
	float robotFacing = 0f;
	float robotStartTurnFacing = 0f;
	int swipeTurnIndex = 0;
	bool swipeTurning = false;
	bool aboutFace = false;
	float gyroSleepTimer = 0f;
	float maxTurnFactor = 1f;
	bool reverseLikeACar = true;
	bool snapHeadingToSelectedObject = true;
	Robot robot = null;
#endregion

#region COMPONENT CALLBACKS

	void OnEnable() {
		//reset default state for this control scheme test
		//Debug.Log("RobotRelativeControls OnEnable");

		lastInputs = Vector2.zero;
		headCommandLastFrame = false;
		moveCommandLastFrame = false;
		debugOverride = false;
		swipeTurning = false;
		aboutFace = false;

		maxTurnFactor = PlayerPrefs.GetFloat("MaxTurnFactor", OptionsScreen.DEFAULT_MAX_TURN_FACTOR);
		reverseLikeACar = PlayerPrefs.GetInt("ReverseLikeACar", OptionsScreen.REVERSE_LIKE_A_CAR) == 1;
		//Debug.Log(gameObject.name + " OnEnable reverseLikeACar("+reverseLikeACar+")");
		snapHeadingToSelectedObject = PlayerPrefs.GetInt("VisionSchemeIndex", 0) == 2;
	}

	void Update() {

		robot = null;
		if(RobotEngineManager.instance != null) robot = RobotEngineManager.instance.current;
		if(robot == null) return;

		robotFacing = MathUtil.ClampAngle(robot.poseAngle_rad * Mathf.Rad2Deg);

		//if coz is picked up, let's zero our wheels and abort control logic
		if(robot.Status(Robot.StatusFlag.IS_PICKED_UP)) {
			robot.DriveWheels(0f, 0f);
			return;
		}


		if(aboutFace) {
			float turnSoFar = MathUtil.AngleDelta(robotStartTurnFacing, robotFacing);
			if(Mathf.Abs(turnSoFar) > 180f) {
				//Debug.Log("frame(" + Time.frameCount + ") EndSwipe turnSoFar(" + turnSoFar + ")");
				aboutFace = false;
				robot.DriveWheels(0f, 0f);
			}
			return;
		}

		if(headAngleStick != null) {

			float headInput = headAngleStick.Vertical;
			if(headCommandLastFrame || headInput != 0f) {
				robot.SetHeadAngle(robot.headAngle_rad + headInput * Time.deltaTime * headAngleChangeRate);
				//headCommandLastFrame = headInput != 0f;
			}
		}

		if(debugOverride) {
			robot.DriveWheels(leftWheelSpeed, rightWheelSpeed);
			RefreshDebugText();
			return;
		}

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		bool driveForwardOnlyMode = false;
		bool driveReverseOnlyMode = false;
		bool turnInPlaceOnlyMode = false;

		float maxAngle = 90f;

		if(verticalStick != null) {

			if(doubleTapTurnAround && verticalStick.DoubleTapped) {
				aboutFace = true;
				robotStartTurnFacing = robotFacing;

				verticalStick.AbsorbDoubleTap();
				robot.DriveWheels(CozmoUtil.MAX_WHEEL_SPEED, -CozmoUtil.MAX_WHEEL_SPEED);
				return;
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

		bool targetLocked = false;
		Vector2 robotForward = robot.Forward;
		Vector2 robotRight = robot.Right;
		if(snapHeadingToSelectedObject && robot.selectedObjects.Count > 0 && robot.selectedObjects[0] != null) {
			targetLocked = true;

			Vector2 atTarget = robot.selectedObjects[0].WorldPosition - robot.WorldPosition;

			float turn = Vector2.Angle(robotForward, atTarget);

			if(turn > targetLockMinTurnAngle) {
				turn = Mathf.Lerp(0f, 1f, turn / targetLockMaxTurnAngle);
				turn = turn*turn;
				inputs.x = turn * (Vector2.Dot(atTarget, robotRight) >= 0f ? 1f: -1f);
				//Debug.Log("frame("+Time.frameCount+") time("+Time.timeSinceLevelLoad+") snapHeadingToSelectedObject turn("+turn+") selected("+robot.selectedObjects[0].ID+") robot.Rotation("+robot.Rotation+")");
			}

			Debug.DrawRay(Vector3.zero, atTarget.normalized * 5f, Color.cyan);
		}
		Debug.DrawRay(Vector3.zero, robotForward * 4f, Color.green);
		Debug.DrawRay(Vector3.zero, robotRight * 4f, Color.yellow);

		if(!targetLocked && horizontalStick != null) {
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
		else if(!robot.Status(Robot.StatusFlag.NONE)) {
			gyroSleepTimer = 0f;
		}
		
		if(!targetLocked && gyroSleepTimer <= gyroSleepTime && gyroInputs != null && gyroInputs.gameObject.activeSelf) { // && (verticalStick == null || verticalStick.IsPressed)) {

			float h = gyroInputs.Horizontal;
			inputs.x += h;
			inputs.x = Mathf.Clamp(inputs.x, -1f, 1f);

			if(gyroInputs.Horizontal != 0f) gyroSleepTimer = 0f; 
		}

		if(inputs.x == 0f && inputs.y == 0f) {
			if(!targetLocked) inputs.x = Input.GetAxis("Horizontal");
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

		robot.DriveWheels(leftWheelSpeed, rightWheelSpeed);

		moveCommandLastFrame = inputs.sqrMagnitude > 0f;

		RefreshDebugText();
	}

	void OnDisable() {
		//clean up this controls test if needed
		//Debug.Log("RobotRelativeControls OnDisable");

		if(RobotEngineManager.instance != null && RobotEngineManager.instance.IsConnected && RobotEngineManager.instance.current != null) {
			RobotEngineManager.instance.current.DriveWheels(0f, 0f);
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
