using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;

public class RobotRelativeControls : MonoBehaviour {

#region INSPECTOR FIELDS

	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] Slider headAngleSlider = null;
	[SerializeField] Slider liftSlider = null;
	[SerializeField] GyroControls gyroInputs = null;
	[SerializeField] float turnInPlaceRollMin = 15f;
	[SerializeField] float turnInPlaceRollMax = 90f;
	[SerializeField] float turnWhileDrivingRollMin = 5f;
	[SerializeField] float turnWhileDrivingRollMax = 90f;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] float swipeTurnAngle = 0f;
	[SerializeField] bool doubleTapTurnAround = true;
	[SerializeField] float headAngleChangeRate = 1f;
	[SerializeField] float targetLockMaxTurnAngle = 25f;
	[SerializeField] float targetLockMinTurnAngle = 4f;
	[SerializeField] float targetLockGyroSwapThreshHold = 0.3f;
	[SerializeField] float targetLockStickSwapThreshHold = 0.1f;

#endregion

#region MISC MEMBERS

	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;
	bool moveCommandLastFrame = false;
	bool debugOverride = false;
	float robotFacing = 0f;
	float robotStartTurnFacing = 0f;
	int swipeTurnIndex = 0;
	bool swipeTurning = false;
	bool aboutFace = false;
	float maxTurnFactor = 1f;
	bool reverseLikeACar = true;
	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }
	bool driveForwardOnlyMode = false;
	bool driveReverseOnlyMode = false;
	bool turnInPlaceOnlyMode = false;
	float maxAngle = 90f;
	float targetLockTimer = 0f;
	ObservedObject targetLock = null;
	ObservedObject lastTargetLock = null;
	float lastGyroX = 0;
	float lastHorStickX = 0;
	float lastHeadStickY = 0;
	float lastDebugAxesX = 0f;
	bool headAngleSliderEngaged = false;
	bool liftSliderEngaged = false;
	float timeSinceInput = 0f;
	Quaternion lastAttitude = Quaternion.identity;

#endregion

#region COMPONENT CALLBACKS

	void OnEnable() {
		//reset default state for this control scheme test
		//Debug.Log("RobotRelativeControls OnEnable");

		lastInputs = Vector2.zero;

		moveCommandLastFrame = false;
		debugOverride = false;
		swipeTurning = false;
		aboutFace = false;

		maxTurnFactor = PlayerPrefs.GetFloat("MaxTurnFactor", OptionsScreen.DEFAULT_MAX_TURN_FACTOR);
		reverseLikeACar = PlayerPrefs.GetInt("ReverseLikeACar", OptionsScreen.REVERSE_LIKE_A_CAR) == 1;
		//Debug.Log(gameObject.name + " OnEnable reverseLikeACar("+reverseLikeACar+")");
		targetLock = null;
		lastTargetLock = null;

		lastGyroX = 0f;
		lastHorStickX = 0f;
		lastHeadStickY = 0f;
		lastDebugAxesX = 0f;
		timeSinceInput = 0f;
		headAngleSliderEngaged = false;
		RefreshHeadAngleSlider();

		liftSliderEngaged = false;
		RefreshLiftSlider();

		ShowSticks();
	}

	void Update() {

		timeSinceInput += Time.deltaTime;

		bool controlsDisabled = robot == null || robot.isBusy;
		controlsDisabled |= GameLayoutTracker.instance != null && GameLayoutTracker.instance.Phase == LayoutTrackerPhase.INVENTORY;

		if(controlsDisabled) {
			ShowSticks(false);
			return;
		}
		else {
			ShowSticks();
		}

		robotFacing = MathUtil.ClampAngle180(robot.poseAngle_rad * Mathf.Rad2Deg);

		if(DebugOverride()) return;

		//if coz is picked up, let's zero our wheels and abort control logic
		if(robot.Status(Robot.StatusFlag.IS_PICKED_UP)) {
			robot.DriveWheels(0f, 0f);
			return;
		}

		if(AboutFace()) return;

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;
		
		driveForwardOnlyMode = false;
		driveReverseOnlyMode = false;
		turnInPlaceOnlyMode = false;
		
		maxAngle = 90f;

		RefreshTargetLock();

		targetLockTimer += Time.deltaTime;
		if(lastTargetLock == null && targetLock != null && robot.Status(Robot.StatusFlag.IS_CARRYING_BLOCK)) targetLockTimer = 0f;

		//if we are unlocked this frame, let's reset our head angle back to sane value
		//if(lastTargetLock != null && targetLock == null && !robot.isBusy) robot.SetHeadAngle();

		bool newLock = lastTargetLock != targetLock;

		lastTargetLock = targetLock;

		if(newLock) {
			lastGyroX = 0f;
			lastHorStickX = 0f;
			lastHeadStickY = 0f;
			lastDebugAxesX = 0f;
		}

		Vector2 targetSwapDirection = Vector2.zero;

		CheckVerticalStick();
		CheckVerticalDebugAxis();

		if(Input.touchCount > 0) {
			timeSinceInput = 0f;
			//Debug.Log("Input.touchCount("+Input.touchCount+") timeSinceInput = 0f;");
		}
		if(Quaternion.Angle(Input.gyro.attitude, lastAttitude) >= 1f) {
			timeSinceInput = 0f;
			//Debug.Log("attitude changed! timeSinceInput = 0f;");
		}

		lastAttitude = Input.gyro.attitude;

		if(!swipeTurning) swipeTurnIndex = 0;

		if(!liftSliderEngaged) RefreshLiftSlider();
		if(!headAngleSliderEngaged) RefreshHeadAngleSlider();

		if(targetLock == null || targetLockTimer < 1f) {
			CheckHorizontalStickTurning();
			CheckGyroTurning();
			CheckDebugHorizontalAxesTurning();

			if(inputs.y != 0 && !headAngleSliderEngaged && !robot.searching && robot.selectedObjects.Count == 0) {
				robot.SetHeadAngle();
			}
		}
		else {
			targetSwapDirection += CheckHorizontalStickTargetSwap();
			targetSwapDirection += CheckGyroTargetSwap();
			targetSwapDirection += CheckDebugAxesTargetSwap();

			if(!newLock) CheckSwapTargetLock(targetSwapDirection);

			//robot.TrackHeadToObject( targetLock );
		}

		bool stopped = inputs.sqrMagnitude == 0f && moveCommandLastFrame;
		if(!stopped) {
			Vector3 delta = inputs - lastInputs;
			if(delta.sqrMagnitude <= 0f) return; // command not changed
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

		if(robot != null && RobotEngineManager.instance.IsConnected) {
			robot.DriveWheels(0f, 0f);
		}

//		if(recorder != null) {
//			recorder.enabled = false;
//		}

		ShowSticks(false);
	}

#endregion

#region PRIVATE METHODS


	void ShowSticks(bool show=true) {
		if(verticalStick != null) verticalStick.gameObject.SetActive(show);
		if(horizontalStick != null) horizontalStick.gameObject.SetActive(show);
		if(headAngleSlider != null) headAngleSlider.gameObject.SetActive(show && (GameLayoutTracker.instance == null || GameLayoutTracker.instance.Phase == LayoutTrackerPhase.DISABLED));
		if(liftSlider != null) liftSlider.gameObject.SetActive(show && (GameLayoutTracker.instance == null || GameLayoutTracker.instance.Phase == LayoutTrackerPhase.DISABLED));
	}

	bool AboutFace() {
		if(!aboutFace) return false;

		float turnSoFar = MathUtil.AngleDelta(robotStartTurnFacing, robotFacing);
		if(Mathf.Abs(turnSoFar) < 180f) return true;
		//Debug.Log("frame(" + Time.frameCount + ") EndSwipe turnSoFar(" + turnSoFar + ")");
		aboutFace = false;
		robot.DriveWheels(0f, 0f);

		return false;
	}

	bool DebugOverride() {
		if(!debugOverride) return false;
		robot.DriveWheels(leftWheelSpeed, rightWheelSpeed);
		RefreshDebugText();
		return true;
	}

	void CheckVerticalStick() {
		if(verticalStick == null) return;
		
		if(doubleTapTurnAround && verticalStick.DoubleTapped) {
			aboutFace = true;
			robotStartTurnFacing = robotFacing;
			
			verticalStick.AbsorbDoubleTap();
			robot.DriveWheels(CozmoUtil.MAX_WHEEL_SPEED_MM, -CozmoUtil.MAX_WHEEL_SPEED_MM);
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

	public bool debugTargetLock = false;
	ObservedObject RefreshTargetLock() {
		targetLock = null;
		//if(!targetLockSelectedObject) return null;
		if(robot.selectedObjects.Count == 0) return null;

		targetLock = robot.targetLockedObject;
		if(targetLock == null) return null;

		Vector2 robotForward = robot.Forward;
		Vector2 robotRight = robot.Right;

		Vector2 atTarget = targetLock.WorldPosition - robot.WorldPosition;
	
		float turn = Vector2.Angle(robotForward, atTarget);
	
		if(turn > targetLockMinTurnAngle) {
			turn = Mathf.Lerp(0f, 1f, turn / targetLockMaxTurnAngle);
			turn = turn * turn;
			inputs.x = turn * Mathf.Sign(Vector2.Dot(atTarget, robotRight));
			if(debugTargetLock) Debug.Log("frame("+Time.frameCount+") time("+Time.timeSinceLevelLoad+") snapHeadingToSelectedObject turn("+turn+") selected("+robot.selectedObjects[0]+") robot.Rotation("+robot.Rotation+")");
		}
	
		Debug.DrawRay(Vector3.zero, atTarget.normalized * 5f, Color.cyan);
		Debug.DrawRay(Vector3.zero, robotForward * 4f, Color.green);
		Debug.DrawRay(Vector3.zero, robotRight * 4f, Color.yellow);

		return targetLock;
	}

	void CheckHorizontalStickTurning() {
		if(horizontalStick == null) return;

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
			
			for(int i=0; i<swipeTurnIndex && i<3; i++) {
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

	Vector2 CheckHorizontalStickTargetSwap() {
		if(horizontalStick == null) return Vector2.zero;
		
		float x = horizontalStick.Horizontal;
		bool swapTriggered = Mathf.Abs(lastHorStickX) < targetLockStickSwapThreshHold && Mathf.Abs(x) >= targetLockStickSwapThreshHold;
		
		lastHorStickX = x;
		
		if(swapTriggered) return x > 0f ? Vector2.right : -Vector2.right;
		return Vector2.zero;
	}

	void CheckGyroTurning() {
		if(gyroInputs == null) return;
		if(timeSinceInput > 5f) return;
		
		float minRoll = turnInPlaceRollMin;
		float maxRoll = turnInPlaceRollMax;

		if(verticalStick != null && verticalStick.IsPressed) {
			minRoll = turnWhileDrivingRollMin;
			maxRoll = turnWhileDrivingRollMax;
		}

		float h = gyroInputs.GetHorizontal(minRoll, maxRoll);

		inputs.x += h;
		inputs.x = Mathf.Clamp(inputs.x, -1f, 1f);

		//if(gyroInputs.Horizontal != 0f) Debug.Log("gyroInputs.Horizontal("+gyroInputs.Horizontal+")");
	}

	Vector2 CheckGyroTargetSwap() {
		if(gyroInputs == null) return Vector2.zero;
		
		float minRoll = turnInPlaceRollMin;
		float maxRoll = turnInPlaceRollMax;
		
		if(verticalStick != null && verticalStick.IsPressed) {
			minRoll = turnWhileDrivingRollMin;
			maxRoll = turnWhileDrivingRollMax;
		}
		
		float h = gyroInputs.GetHorizontal(minRoll, maxRoll);

		bool swapTriggered = Mathf.Abs(lastGyroX) < targetLockGyroSwapThreshHold && Mathf.Abs(h) >= targetLockGyroSwapThreshHold;

		lastGyroX = h;

		if(swapTriggered) return h > 0f ? Vector2.right : -Vector2.right;
		return Vector2.zero;
	}

	void CheckVerticalDebugAxis() {
		if(inputs.y == 0f) {
			inputs.y = Input.GetKey(KeyCode.W) ? 1f : Input.GetKey(KeyCode.S) ? -1f : 0f;
		}
	}

	void CheckDebugHorizontalAxesTurning() {
		if(inputs.x == 0f) {
			inputs.x = Input.GetKey(KeyCode.D) ? 1f : Input.GetKey(KeyCode.A) ? -1f : 0f;
		}
	}

	Vector2 CheckDebugAxesTargetSwap() {
		float x = Input.GetKey(KeyCode.D) ? 1f : Input.GetKey(KeyCode.A) ? -1f : 0f;

		bool horSwapTriggered = Mathf.Abs(lastDebugAxesX) < targetLockStickSwapThreshHold && Mathf.Abs(x) >= targetLockStickSwapThreshHold;

		lastDebugAxesX = x;

		Vector2 ret = Vector2.zero;
		if(horSwapTriggered) ret += x > 0f ? Vector2.right : -Vector2.right;

		return ret;
	}

	void CheckSwapTargetLock(Vector2 direction) {
		if(direction.sqrMagnitude == 0f) return; 
		if(robot != null && robot.pertinentObjects.Count <= 1) return;

		List<ObservedObject> pertinentObjects = robot.pertinentObjects;
		ObservedObject oldLock = robot.targetLockedObject;
		Vector3 targetPos = oldLock.WorldPosition;
		Vector3 atTarget = (targetPos - robot.WorldPosition).normalized;
		Vector3 flatAtTarget = ((Vector2)atTarget).normalized;
		Vector3 crossAt = Vector3.Cross(flatAtTarget.normalized, Vector3.forward);

		List<ObservedObject> potential;

		///sift and sort
		if(direction.x != 0) {
			potential = pertinentObjects.FindAll(obj => TargetIsInDirectionFromTargetLock(obj, oldLock, direction.x > 0 ? crossAt : -crossAt) );

			if(potential == null || potential.Count == 0) return;

			potential.Sort( ( obj1, obj2 ) => {

				Vector3 at1 = (obj1.WorldPosition - robot.WorldPosition).normalized;
				Vector3 at2 = (obj2.WorldPosition - robot.WorldPosition).normalized;

				Vector3 at1AlongGround = ((Vector2)at1).normalized;
				Vector3 at2AlongGround = ((Vector2)at2).normalized;
				
				float angle1 = Vector3.Angle(flatAtTarget, at1AlongGround);
				float angle2 = Vector3.Angle(flatAtTarget, at2AlongGround);

				return angle1.CompareTo(angle2);   
			} );

		}
		else {
			potential = pertinentObjects.FindAll(obj => TargetIsInDirectionFromTargetLock(obj, oldLock, direction.y > 0 ? Vector3.forward : -Vector3.forward) );

			if(potential == null || potential.Count == 0) return;

			potential.Sort( ( obj1, obj2 ) => {
				
				Vector3 at1 = (obj1.WorldPosition - robot.WorldPosition).normalized;
				Vector3 at2 = (obj2.WorldPosition - robot.WorldPosition).normalized;
				
				Vector3 at1AlongGround = ((Vector2)at1).normalized;
				Vector3 at2AlongGround = ((Vector2)at2).normalized;
				
				float angle1 = Vector3.Angle(at1AlongGround, at1);
				float angle2 = Vector3.Angle(at2AlongGround, at2);
				
				return angle1.CompareTo(angle2);   
			} );
		}

		//robot.selectedObjects.Clear();
		/*robot.selectedObjects.Remove(potential[0]);
		robot.selectedObjects.Insert(0, potential[0]);*/
		robot.targetLockedObject = potential[0];
		targetLock = potential[0];
		//Debug.Log("frame("+Time.frameCount+") swapped oldLock("+oldLock+") newLock("+targetLock+", "+targetLock.ObjectType+", "+targetLock.Family+", "+targetLock.WorldPosition+") direction("+direction+") from potential("+potential.Count+")");
	}

	bool TargetIsInDirectionFromTargetLock(ObservedObject obj, ObservedObject locked, Vector3 direction) {
		if(obj == locked) return false;

		Vector3 atLock = (locked.WorldPosition - robot.WorldPosition).normalized;
		Vector3 atObj = (obj.WorldPosition - robot.WorldPosition).normalized;
		Vector3 delta = atObj - atLock;

		return Vector3.Dot(delta, direction) > 0f;
	}

	//reset our slider to our current head angle
	void RefreshHeadAngleSlider() {
		if(headAngleSlider == null) return;
		if(robot == null) return;
		
		headAngleSlider.onValueChanged.RemoveListener(HeadAngleSliderChanged);
		headAngleSlider.value = robot.GetHeadAngleFactor();
		headAngleSlider.onValueChanged.AddListener(HeadAngleSliderChanged);
	}
	
	void HeadAngleSliderChanged(float val) {
		if(robot == null) return;
		
		robot.SetHeadAngle(val);
	}

	//reset our lifter to our current height
	void RefreshLiftSlider() {
		if(liftSlider == null) return;
		if(robot == null) return;

		liftSlider.onValueChanged.RemoveListener(LiftSliderChanged);
		liftSlider.value = robot.GetLiftHeightFactor();
		liftSlider.onValueChanged.AddListener(LiftSliderChanged);
	}

	void LiftSliderChanged(float val) {
		if(robot == null) return;
		
		robot.SetLiftHeight(val);
	}

	void RefreshDebugText() {
		if(text_x != null) text_x.text = "x(" + inputs.x.ToString("N") + ")";
		if(text_y != null) text_y.text = "y(" + inputs.y.ToString("N") + ")";
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

	public void LiftSliderEnaged(bool engaged) {
		liftSliderEngaged = engaged;
	}

	public void HeadAngleSliderEnaged(bool engaged) {
		headAngleSliderEngaged = engaged;
	}


#endregion

}
