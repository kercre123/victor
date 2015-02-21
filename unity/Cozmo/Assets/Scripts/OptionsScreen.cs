using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;

	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;

	void OnEnable () {

		Init();

		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}
	}

	void Init () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.value = PlayerPrefs.GetFloat("MaxTurnFactor", DEFAULT_MAX_TURN_FACTOR);
		}
	}

	void OnDisable () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) slider_turnSpeed.onValueChanged.RemoveListener(MaxTurnSpeedChanged);
	}

	public void MaxTurnSpeedChanged (float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}

	public void ResetToDefaultSettings () {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		Init();
	}
}
