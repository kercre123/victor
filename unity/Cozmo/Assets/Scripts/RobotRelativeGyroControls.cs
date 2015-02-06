using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeGyroControls : MonoBehaviour {

	[SerializeField] Transform robot = null;
	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;
	[SerializeField] Text turnOnlyLabel;
	[SerializeField] SpringSlider slider;

	float rollStart = 0f;
	float pitchStart = 0f;
	ScreenOrientation orientation = ScreenOrientation.Unknown;

	Vector2 inputs = Vector2.zero;
	Vector2 lastInputs = Vector2.zero;
	float timeSinceLastCommand = 0f;
	float refreshTime = 0.1f;
	
	float leftWheelSpeed = 0f;
	float rightWheelSpeed = 0f;

	bool turnOnly = true;

	void OnEnable() {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log("RobotRelativeGyroControls OnEnable");

		Calibrate();

		turnOnlyLabel.text = "TurnOnly";
		slider.gameObject.SetActive(true);
	}

	void FixedUpdate() {
		timeSinceLastCommand += Time.deltaTime;
		
		if(timeSinceLastCommand < refreshTime)
			return;
		
		timeSinceLastCommand = 0f;
		//take our gyro control axes and translate to robot
		if(!SystemInfo.supportsGyroscope) return;
		
		if(orientation != Screen.orientation) Calibrate();
		
		//figure out axes based on screen orientation mode
		
		//switch(Screen.orientation) {
		//	case ScreenOrientation.AutoRotation:
		//		break;
		//	case ScreenOrientation.Portrait:
		//		break;
		//	case ScreenOrientation.PortraitUpsideDown:
		//		break;
		//	//case ScreenOrientation.Landscape: //same value as landscapeleft
		//	case ScreenOrientation.LandscapeLeft:
		//		break;
		//	case ScreenOrientation.LandscapeRight:
		//		break;
		//}

		inputs = Vector2.zero;
		
		Quaternion deviceRot = Input.gyro.attitude;
		Vector3 euler = deviceRot.eulerAngles;
		
		float roll = euler.z - rollStart;
		while(roll < -180f)
			roll += 360f;
		while(roll > 180f)
			roll -= 360f;

		inputs.x = Mathf.Clamp01((Mathf.Abs(roll) - 10f) / 45f);
		if(roll >= 0f)
			inputs.x = -inputs.x;

		float pitch = 0f;

		if(turnOnly) {
			inputs.y = slider.value;
		}
		else {

			pitch = euler.y - pitchStart;
			while(pitch < -180f)
				pitch += 360f;
			while(pitch > 180f)
				pitch -= 360f;

			inputs.y = Mathf.Clamp01((Mathf.Abs(pitch) - 10f) / 45f);
			if(pitch < 0f)
				inputs.y = -inputs.y;
		}
		
		Debug.Log("RobotRelativeControls gyro euler(" + euler + ") roll(" + roll + ") pitch(" + pitch + ") x(" + inputs.x + ") y(" + inputs.y + ")");

		
		if((inputs - lastInputs).magnitude < 0.1f) return;
		lastInputs = inputs;
		
		if(Intro.CurrentRobotID != 0) {
			RobotEngineManager.CalcWheelSpeedsFromBotRelativeInputs(inputs, out leftWheelSpeed, out rightWheelSpeed);
			RobotEngineManager.instance.DriveWheels(Intro.CurrentRobotID, leftWheelSpeed, rightWheelSpeed);
			return;
		}
		
		//no robot, fake it in unity visualization
		if(inputs.x == 0f && inputs.y == 0f)
			return;
		
		float turn = Mathf.Min(maxTurn * Time.deltaTime, Mathf.Abs(inputs.x) * maxTurn);
		if(inputs.x < 0f)
			turn = -turn;
		robot.rotation *= Quaternion.AngleAxis(turn, robot.up);
		
		Vector3 idealMove = robot.forward * inputs.y * maxVel;
		robot.position += idealMove * Time.deltaTime;
	}


	void OnDisable() {
		//clean up this controls test if needed
		Debug.Log("RobotRelativeGyroControls OnDisable");
	}

	void OnGUI() {
		GUILayout.BeginVertical();
		GUILayout.Space(100);
		GUILayout.Label("input("+inputs+")");
		GUILayout.Label("leftWheelSpeed("+leftWheelSpeed+")");
		GUILayout.Label("rightWheelSpeed("+rightWheelSpeed+")");
		GUILayout.Label("Gyroscope attitudeEulers : " + Input.gyro.attitude.eulerAngles);
		GUILayout.Label("Gyroscope attitude : " + Input.gyro.attitude);
		GUILayout.Label("Gyroscope gravity : " + Input.gyro.gravity);
		GUILayout.Label("Gyroscope rotationRate : " + Input.gyro.rotationRate);
		GUILayout.Label("Gyroscope rotationRateUnbiased : " + Input.gyro.rotationRateUnbiased);
		GUILayout.Label("Gyroscope updateInterval : " + Input.gyro.updateInterval);
		GUILayout.Label("Gyroscope userAcceleration : " + Input.gyro.userAcceleration);
		GUILayout.Label("ScreenOrientation : " + Screen.orientation);
		GUILayout.EndVertical();
	}

	public void Calibrate() {
		if(!SystemInfo.supportsGyroscope)
			return;
		Debug.Log("ScreenOrientation->(" + Screen.orientation + ")");
		orientation = Screen.orientation;
		Input.gyro.enabled = true;
		Quaternion deviceRot = Input.gyro.attitude;
		Vector3 euler = deviceRot.eulerAngles;
		rollStart = euler.z;
		pitchStart = euler.y;
	}

	public void TurnOnlyToggle() {
		turnOnly = !turnOnly;
		slider.gameObject.SetActive(turnOnly);
		turnOnlyLabel.text = turnOnly ? "TurnOnly" : "FullGyro";
	}
}
