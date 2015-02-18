using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] GyroControls gyroInputs = null;
	[SerializeField] AccelControls accelInputs = null;
	[SerializeField] Toggle gyroRollControl = null;
	[SerializeField] Toggle gyroPitchControl = null;

	[SerializeField] Text text_x = null;
	[SerializeField] Text text_y = null;

	[SerializeField] Text text_leftWheelSpeed = null;
	[SerializeField] Text text_rightWheelSpeed = null;

	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0f;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;

	//bool reverseLikeACar = false;
	bool moveCommandLastFrame = false;

	void OnEnable() {
		//acquire the robot
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
	}

	void FixedUpdate() {
		timeSinceLastCommand += Time.deltaTime;

		if(timeSinceLastCommand < refreshTime)
			return;

		timeSinceLastCommand = 0f;

		//take our v-pad control axes and calc translate to robot
		inputs = Vector2.zero;

		bool driveForwardOnlyMode = false;
		bool driveReverseOnlyMode = false;
		bool turnInPlaceOnlyMode = false;

		if(horizontalStick != null) {
			if(horizontalStick.SideModeEngaged) {
				turnInPlaceOnlyMode = true;
			}
			inputs.x = horizontalStick.Horizontal;
		}

		float maxAngle = 135f;

		if(verticalStick != null) {

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

//		if(inputs.x == 0f && inputs.y == 0f) {
//			inputs.x = Input.GetAxis("Horizontal");
//			inputs.y = Input.GetAxis("Vertical");
//		}

		if(gyroInputs != null && gyroInputs.gameObject.activeSelf && (verticalStick == null || verticalStick.IsPressed)) {
			inputs.x = gyroInputs.Horizontal;

//			if(gyroPitchControl != null && gyroPitchControl.isOn) {
//				inputs.y = gyroInputs.Vertical;
//			}
		}

		if(accelInputs != null && accelInputs.gameObject.activeSelf && (verticalStick == null || verticalStick.IsPressed) ) {
			inputs.x = accelInputs.Horizontal;
		}

		bool stopped = inputs.sqrMagnitude == 0f && moveCommandLastFrame;
		if(!stopped) {
			Vector3 delta = inputs - lastInputs;
			if(delta.sqrMagnitude <= 0f) return; // command not changed
		}

		lastInputs = inputs;

		if(driveForwardOnlyMode) {
			CozmoUtil.CalcDriveWheelSpeedsForInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, false);
		}
		else if(driveReverseOnlyMode) {
			CozmoUtil.CalcDriveWheelSpeedsForInputs(inputs, out leftWheelSpeed, out rightWheelSpeed, maxAngle, true);
		}
		else if(turnInPlaceOnlyMode) {
			CozmoUtil.CalcTurnInPlaceWheelSpeeds(inputs.x, out leftWheelSpeed, out rightWheelSpeed);
		}
		else { //continues input range mode...causes issues at thresholds

			CozmoUtil.CalcWheelSpeedsFromBotRelativeInputsB(inputs, out leftWheelSpeed, out rightWheelSpeed);
		}

		if(RobotEngineManager.instance != null && Intro.CurrentRobotID != 0) {
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
		}

		moveCommandLastFrame = inputs.sqrMagnitude > 0f;


		if(text_x != null) text_x.text = "x(" + inputs.x.ToString("N") + ")";
		if(text_y != null) text_y.text = "y(" + inputs.y.ToString("N") + ")";

		if(text_leftWheelSpeed != null) text_leftWheelSpeed.text = "L(" + leftWheelSpeed.ToString("N") + ")";
		if(text_rightWheelSpeed != null) text_rightWheelSpeed.text = "R(" + rightWheelSpeed.ToString("N") + ")";
	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("RobotRelativeControls OnDisable");

		if(RobotEngineManager.instance != null && RobotEngineManager.instance.IsConnected) {
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, 0f, 0f);
		}
	}

//	void OnGUI() {
//		GUILayout.BeginArea(new Rect(Screen.width*0.5f-150f, 300f, 300f, 300f));
//		GUILayout.Label("input("+inputs+")");
//		GUILayout.Label("leftWheelSpeed("+leftWheelSpeed+")");
//		GUILayout.Label("rightWheelSpeed("+rightWheelSpeed+")");
//		GUILayout.EndArea();
//	}

	
	public void CalibrateGyro() {
		if(gyroInputs != null) gyroInputs.Calibrate();
	}
	
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

}
