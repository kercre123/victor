using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GyroControls : MonoBehaviour {

	[SerializeField] float rollAngleMin = 10f;
	[SerializeField] float rollAngleMax = 45f;

	[SerializeField] float pitchAngleMin = 10f;
	[SerializeField] float pitchAngleMax = 45f;

	[SerializeField] RectTransform upIndicator = null;

	float x = 0f;
	public float GetHorizontal(float min=-1f, float max=-1f) {
		if (min < 0) min = rollAngleMin;
		if (max < 0) min = rollAngleMax;

		if(!SystemInfo.supportsGyroscope) return 0f;

		float roll = MathUtil.ClampAngle180(MSP_Input.GyroAccel.GetRoll());
		x = Mathf.Clamp01((Mathf.Abs(roll) - rollAngleMin) / (rollAngleMax - rollAngleMin)) * -Mathf.Sign(roll);
		return x; 
	}

	float y = 0f;
	public float GetVertical(float min=-1f, float max=-1f) {
		if(!SystemInfo.supportsGyroscope) return 0f;

		if (min < 0) min = pitchAngleMin;
		if (max < 0) min = pitchAngleMax;

		float pitch = MSP_Input.GyroAccel.GetPitch();
		y = Mathf.Clamp01((Mathf.Abs(pitch) - pitchAngleMin) / (pitchAngleMax - pitchAngleMin)) * -Mathf.Sign(pitch);

		return y; 
	}

	void FixedUpdate() {
		if(!SystemInfo.supportsGyroscope) return;


		if(upIndicator != null) {
			float roll = MSP_Input.GyroAccel.GetRoll();
			upIndicator.localRotation = Quaternion.AngleAxis(-roll, Vector3.forward);
		}
	}

}
