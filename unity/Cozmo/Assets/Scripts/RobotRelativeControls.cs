using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeControls : MonoBehaviour {

	[SerializeField] VirtualStick verticalStick = null;
	[SerializeField] VirtualStick horizontalStick = null;
	[SerializeField] GyroControls gyroInputs = null;
	[SerializeField] Toggle gyroRollControl = null;
	[SerializeField] Toggle gyroPitchControl = null;

	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0f;
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;

	void OnEnable() {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log("RobotRelativeControls OnEnable");
		if(gyroRollControl != null) {
			gyroRollControl.isOn = false;
			gyroRollControl.gameObject.SetActive(gyroInputs != null);
		}

		if(gyroPitchControl != null) {
			gyroPitchControl.isOn = false;
			gyroPitchControl.gameObject.SetActive(gyroInputs != null);
		}
	}

	void FixedUpdate() {
		timeSinceLastCommand += Time.deltaTime;

		if(timeSinceLastCommand < refreshTime)
			return;

		timeSinceLastCommand = 0f;

		//take our v-pad control axes and calc translate to robot
		inputs = new Vector2(horizontalStick.Horizontal, verticalStick.Vertical);

//		if(inputs.x == 0f && inputs.y == 0f) {
//			inputs.x = Input.GetAxis("Horizontal");
//			inputs.y = Input.GetAxis("Vertical");
//		}

		if(gyroInputs != null) {
			if(gyroRollControl != null && gyroRollControl.isOn) inputs.x = gyroInputs.Horizontal;
			if(gyroPitchControl != null && gyroPitchControl.isOn) inputs.y = gyroInputs.Vertical;
		}

		if((inputs - lastInputs).magnitude < 0.1f) return;
		lastInputs = inputs;

		if(Intro.CurrentRobotID != 0) {
			RobotEngineManager.CalcWheelSpeedsFromBotRelativeInputs(inputs, out leftWheelSpeed, out rightWheelSpeed);
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
		}
	}

	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("RobotRelativeControls OnDisable");
	}

	void OnGUI() {
		GUILayout.BeginArea(new Rect(Screen.width*0.5f-150f, 300f, 300f, 300f));
		GUILayout.Label("input("+inputs+")");
		GUILayout.Label("leftWheelSpeed("+leftWheelSpeed+")");
		GUILayout.Label("rightWheelSpeed("+rightWheelSpeed+")");
		GUILayout.EndArea();
	}

	
	public void CalibrateGyro() {
		if(gyroInputs != null) gyroInputs.Calibrate();
	}
	
	public void ToggleRollControl() {
		if(gyroRollControl != null) gyroRollControl.isOn = !gyroRollControl.isOn;
	}

	public void TogglePitchControl() {
		if(gyroPitchControl != null) gyroPitchControl.isOn = !gyroPitchControl.isOn;
	}

}
