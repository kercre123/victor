using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GyroControls : MonoBehaviour {

	[SerializeField] float rollAngleMin = 10f;
	[SerializeField] float rollAngleMax = 45f;

	[SerializeField] float pitchAngleMin = 10f;
	[SerializeField] float pitchAngleMax = 45f;

	[SerializeField] RectTransform upIndicator = null;

	float rollStart = 270f;
	float pitchStart = 0f;
	//ScreenOrientation orientation = ScreenOrientation.Unknown;

	float x = 0f;
	float lastXStamp = 0f;
	public float Horizontal {
		get {
			if(!SystemInfo.supportsGyroscope) return 0f;

			if(Time.frameCount == lastXStamp) return x;
			lastXStamp = Time.frameCount;

			Quaternion deviceRot = Input.gyro.attitude;
			Vector3 euler = deviceRot.eulerAngles;
			
			float roll = euler.z - rollStart;
			while(roll < -180f) roll += 360f;
			while(roll > 180f) roll -= 360f;
			
			x = Mathf.Clamp01((Mathf.Abs(roll) - rollAngleMin) / (rollAngleMax - rollAngleMin));
			if(roll >= 0f) x = -x;

			return x; 
		}
	}

	float y = 0f;
	float lastYStamp = 0f;
	public float Vertical {
		get { 
			if(!SystemInfo.supportsGyroscope) return 0f;

			if(Time.frameCount == lastYStamp) return y;
			lastYStamp = Time.frameCount;
			
			Quaternion deviceRot = Input.gyro.attitude;
			Vector3 euler = deviceRot.eulerAngles;
			
			float pitch = euler.y - pitchStart;
			while(pitch < -180f) pitch += 360f;
			while(pitch > 180f) pitch -= 360f;
			
			y = Mathf.Clamp01((Mathf.Abs(pitch) - pitchAngleMin) / (pitchAngleMax - pitchAngleMin));
			if(pitch < 0f) y = -y;
			
			return y; 
		}
	}

//	void OnEnable() {
//		Calibrate();
//	}

	void FixedUpdate() {
		if(!SystemInfo.supportsGyroscope) return;


		if(upIndicator != null) {
			Quaternion deviceRot = Input.gyro.attitude;
			Vector3 euler = deviceRot.eulerAngles;
			
			float roll = euler.z - rollStart;
			while(roll < -180f) roll += 360f;
			while(roll > 180f) roll -= 360f;

			upIndicator.localRotation = Quaternion.AngleAxis(-roll, Vector3.forward);
		}
	}

//	void OnGUI() {
//		GUILayout.BeginArea(new Rect(Screen.width*0.5f-150f, Screen.height*0.5f, 300f, 300f));
//		GUILayout.Label("x("+x+")");
//		GUILayout.Label("y("+y+")");
//		GUILayout.Label("Gyroscope attitudeEulers : " + Input.gyro.attitude.eulerAngles);
//		GUILayout.Label("Gyroscope attitude : " + Input.gyro.attitude);
//		GUILayout.Label("Gyroscope gravity : " + Input.gyro.gravity);
//		GUILayout.Label("Gyroscope rotationRate : " + Input.gyro.rotationRate);
//		GUILayout.Label("Gyroscope rotationRateUnbiased : " + Input.gyro.rotationRateUnbiased);
//		GUILayout.Label("Gyroscope updateInterval : " + Input.gyro.updateInterval);
//		GUILayout.Label("Gyroscope userAcceleration : " + Input.gyro.userAcceleration);
//		GUILayout.Label("ScreenOrientation : " + Screen.orientation);
//		GUILayout.EndArea();
//	}

	public void Calibrate() {
		if(!SystemInfo.supportsGyroscope) return;
		//orientation = Screen.orientation;
		Input.gyro.enabled = true;
		Quaternion deviceRot = Input.gyro.attitude;
		Vector3 euler = deviceRot.eulerAngles;
		rollStart = euler.z;
		pitchStart = euler.y;
		Debug.Log("GyronControls Calibrate ScreenOrientation(" + Screen.orientation + ") rollStart(" + rollStart + ") pitchStart(" + pitchStart + ")");

	}
}
