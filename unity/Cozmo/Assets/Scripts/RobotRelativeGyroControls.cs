using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class RobotRelativeGyroControls : MonoBehaviour {

	[SerializeField] Transform robot = null;

	[SerializeField] float maxVel = 10f;
	[SerializeField] float maxTurn = 90f;

	float rollStart = 0f;
	float pitchStart = 0f;
	ScreenOrientation orientation = ScreenOrientation.Unknown;

	void OnEnable () {
		//acquire the robot
		//reset default state for this control scheme test
		Debug.Log ("RobotRelativeGyroControls OnEnable");

		Calibrate();
	}

	void Update () {
		//take our gyro control axes and translate to robot
		if (!SystemInfo.supportsGyroscope) return;

		if (orientation != Screen.orientation) Calibrate ();

		//figure out axes based on screen orientation mode

		switch (Screen.orientation) {
			case ScreenOrientation.AutoRotation:
				break;
			case ScreenOrientation.Portrait:
				break;
			case ScreenOrientation.PortraitUpsideDown:
				break;
			//case ScreenOrientation.Landscape: //same value as landscapeleft
			case ScreenOrientation.LandscapeLeft:
				break;
			case ScreenOrientation.LandscapeRight:
				break;
		}

		float x = 0f;
		float z = 0f;

		Quaternion deviceRot = Input.gyro.attitude;
		Vector3 euler = deviceRot.eulerAngles;

		float roll = euler.z - rollStart;
		while(roll < -180f) roll += 360f;
		while(roll > 180f) roll -= 360f;

		float pitch = euler.y - pitchStart;
		while(pitch < -180f) pitch += 360f;
		while(pitch > 180f) pitch -= 360f;

		x = Mathf.Clamp01( (Mathf.Abs(roll) - 10f) / 45f);
		if(roll >= 0f) x = -x;

		z = Mathf.Clamp01( (Mathf.Abs(pitch) - 10f) / 45f);
		if(pitch < 0f) z = -z;

		Debug.Log ("RobotRelativeControls gyro euler("+euler+") roll("+roll+") pitch("+pitch+") z("+z+") x("+x+")");

		if (x == 0f && z == 0f) return;

		float turn = Mathf.Min (maxTurn * Time.deltaTime, Mathf.Abs(x) * maxTurn);
		if (x < 0f) turn = -turn;
		robot.rotation *= Quaternion.AngleAxis (turn, robot.up);

		Vector3 idealMove = robot.forward * z * maxVel;
		robot.position += idealMove * Time.deltaTime;
	}

	void OnDisable () {
		//clean up this controls test if needed
		Debug.Log ("RobotRelativeGyroControls OnDisable");
	}

	void OnGUI () {
		GUILayout.BeginVertical();
		GUILayout.Space(100);
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
		if (!SystemInfo.supportsGyroscope) return;
		Debug.Log ("ScreenOrientation->("+Screen.orientation+")");
		orientation = Screen.orientation;
		Input.gyro.enabled = true;
		Quaternion deviceRot = Input.gyro.attitude;
		Vector3 euler = deviceRot.eulerAngles;
		rollStart = euler.z;
		pitchStart = euler.y;
	}
}
