using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle cozmoVision;

	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;
	private const int DEFAULT_COZMO_VISION = 0;

	void OnEnable () {

		Init();

		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}

		if(cozmoVision != null && cozmoVision.gameObject.activeSelf) {
			cozmoVision.onValueChanged.AddListener(CozmoVisionChanged);
		}
	}

	void Init () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.value = PlayerPrefs.GetFloat("MaxTurnFactor", DEFAULT_MAX_TURN_FACTOR);
		}
		if(cozmoVision !=null) {
			cozmoVision.isOn = PlayerPrefs.GetInt("CozmoVision", DEFAULT_COZMO_VISION) == 1;
		}
	}

	void OnDisable () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) slider_turnSpeed.onValueChanged.RemoveListener(MaxTurnSpeedChanged);
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

	public void ResetToDefaultSettings () {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		PlayerPrefs.DeleteKey("CozmoVision");
		Init();
	}
}
