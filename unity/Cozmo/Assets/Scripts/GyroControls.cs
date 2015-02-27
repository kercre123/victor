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
	public float Horizontal {
		get {
			if(!SystemInfo.supportsGyroscope) return 0f;

			float roll = CozmoUtil.ClampAngle(MSP_Input.GyroAccel.GetRoll());
			x = Mathf.Clamp01((Mathf.Abs(roll) - rollAngleMin) / (rollAngleMax - rollAngleMin)) * (roll >= 0f ? -1f : 1f);
			return x; 
		}
	}

	float y = 0f;
	public float Vertical {
		get { 
			if(!SystemInfo.supportsGyroscope) return 0f;

			float pitch = MSP_Input.GyroAccel.GetPitch();
			y = Mathf.Clamp01((Mathf.Abs(pitch) - pitchAngleMin) / (pitchAngleMax - pitchAngleMin)) * (pitch >= 0f ? -1f : 1f);

			return y; 
		}
	}

	void FixedUpdate() {
		if(!SystemInfo.supportsGyroscope) return;


		if(upIndicator != null) {
			float roll = MSP_Input.GyroAccel.GetRoll();
			upIndicator.localRotation = Quaternion.AngleAxis(-roll, Vector3.forward);
		}
	}

}
