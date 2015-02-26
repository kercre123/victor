using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle toggle_cozmoVision;
	[SerializeField] Toggle toggle_cozmoVision2;

	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;
	private const int DEFAULT_COZMO_VISION = 0;

	void OnEnable () {

		Init();

		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}

		if(toggle_cozmoVision != null && toggle_cozmoVision.gameObject.activeSelf) {
			toggle_cozmoVision.onValueChanged.AddListener(CozmoVisionChanged);
		}

		if(toggle_cozmoVision2 != null && toggle_cozmoVision2.gameObject.activeSelf) {
			toggle_cozmoVision2.onValueChanged.AddListener(CozmoVision2Changed);
		}
	}

	void Init () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.value = PlayerPrefs.GetFloat("MaxTurnFactor", DEFAULT_MAX_TURN_FACTOR);
		}
		if(toggle_cozmoVision !=null) {
			toggle_cozmoVision.isOn = PlayerPrefs.GetInt("CozmoVision", DEFAULT_COZMO_VISION) == 1;
		}
		if(toggle_cozmoVision2 !=null) {
			toggle_cozmoVision2.isOn = PlayerPrefs.GetInt("CozmoVision2", DEFAULT_COZMO_VISION) == 1;
		}
	}

	void OnDisable () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) 
			slider_turnSpeed.onValueChanged.RemoveListener(MaxTurnSpeedChanged);
		if(toggle_cozmoVision != null && toggle_cozmoVision.gameObject.activeSelf) 
			toggle_cozmoVision.onValueChanged.RemoveListener(CozmoVisionChanged);
		if(toggle_cozmoVision2 != null && toggle_cozmoVision2.gameObject.activeSelf) 
			toggle_cozmoVision2.onValueChanged.RemoveListener(CozmoVision2Changed);
	}

	public void MaxTurnSpeedChanged (float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}

	public void CozmoVisionChanged (bool val) {
		if(val) {
			PlayerPrefs.SetInt("CozmoVision", 1);
		} else {
			PlayerPrefs.SetInt("CozmoVision", 0);
		}
	}

	public void CozmoVision2Changed (bool val) {
		if(val) {
			PlayerPrefs.SetInt("CozmoVision2", 1);
		} else {
			PlayerPrefs.SetInt("CozmoVision2", 0);
		}
	}

	public void ResetToDefaultSettings () {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		PlayerPrefs.DeleteKey("CozmoVision");
		PlayerPrefs.DeleteKey("CozmoVision2");
		Init();
	}
}
