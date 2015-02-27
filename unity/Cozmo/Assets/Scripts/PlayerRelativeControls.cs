using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class PlayerRelativeControls : MonoBehaviour {
	
	#region INSPECTOR FIELDS
	[SerializeField] VirtualStick stick = null;
	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;
	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;

	#endregion
	
	#region MISC MEMBERS
	Vector2 inputs = Vector2.zero;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;
	bool moveCommandLastFrame = false;
	float maxTurnFactor = 1f;
	float robotFacing = 0f;
	float robotStartHeading = 0f;
	float compassStartHeading = 0f;
	#endregion
	
	#region COMPONENT CALLBACKS
	
	void OnEnable() {
		//reset default state for this control scheme test
		Debug.Log("PlayerRelativeControls OnEnable");

		moveCommandLastFrame = false;
		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			robotStartHeading = CozmoUtil.ClampAngle(RobotEngineManager.instance.current.poseAngle_rad * Mathf.Rad2Deg);
				//Debug.Log("frame("+Time.frameCount+") robotFacing(" + robotFacing + ")");
		}

		compassStartHeading = CozmoUtil.ClampAngle(Input.compass.trueHeading);
		Debug.Log("frame("+Time.frameCount+") robotStartHeading(" + robotStartHeading + ") compassStartHeading(" + compassStartHeading + ")");
		maxTurnFactor = PlayerPrefs.GetFloat("MaxTurnFactor", OptionsScreen.DEFAULT_MAX_TURN_FACTOR);
	}
	
	void FixedUpdate() {
		
		//bool robotFacingStale = true;
		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
			robotFacing = CozmoUtil.ClampAngle(RobotEngineManager.instance.current.poseAngle_rad * Mathf.Rad2Deg);
			//robotFacingStale = false;
			//Debug.Log("frame("+Time.frameCount+") robotFacing(" + robotFacing + ")");
		}

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		if(stick != null) {
			inputs.x = stick.Horizontal;
			inputs.y = stick.Vertical;
		}

		if(!moveCommandLastFrame && inputs.sqrMagnitude == 0f) {
			return; // command not changed
		}

		float relativeScreenFacing = CozmoUtil.AngleDelta(compassStartHeading, CozmoUtil.ClampAngle(Input.compass.trueHeading));
		float relativeRobotFacing = CozmoUtil.AngleDelta(robotStartHeading, robotFacing);

		float screenToRobot = CozmoUtil.AngleDelta(relativeScreenFacing, relativeRobotFacing);
		float throwAngle = Vector2.Angle(Vector2.up, inputs.normalized) * (Vector2.Dot(inputs.normalized, Vector2.right) >= 0f ? -1f : 1f);
		float relativeAngle = CozmoUtil.AngleDelta(screenToRobot, throwAngle);

		Debug.Log("RobotRelativeControls relativeScreenFacing("+relativeScreenFacing+") relativeRobotFacing("+relativeRobotFacing+") screenToRobot("+screenToRobot+") throwAngle("+throwAngle+") relativeAngle("+relativeAngle+")");
		inputs = Quaternion.AngleAxis(-screenToRobot, Vector3.forward) * inputs;

		if(Mathf.Abs(relativeAngle) > 30f) {
			inputs.x = Mathf.Clamp01(Mathf.Abs(relativeAngle) / 90f) * (relativeAngle >= 0f ? -1f : 1f);
			CozmoUtil.CalcTurnInPlaceWheelSpeeds(inputs.x, out leftWheelSpeed, out rightWheelSpeed, 1f);
		}
		else { //continuous input range mode...causes issues at thresholds
			CozmoUtil.CalcWheelSpeedsForThumbStickInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, 180f, maxTurnFactor);
		}
		
		if(RobotEngineManager.instance != null && RobotEngineManager.instance.current != null) {
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

	#endregion
	
}
