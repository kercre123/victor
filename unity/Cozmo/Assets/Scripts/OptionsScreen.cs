using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle[] toggle_cozmoVisions;

	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;
	private const int DEFAULT_COZMO_VISION = 0;

	void OnEnable () {

		Init();

		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}

		if(toggle_cozmoVisions.Length > 0 && toggle_cozmoVisions[0] != null && toggle_cozmoVisions[0].gameObject.activeSelf) {
			toggle_cozmoVisions[0].onValueChanged.AddListener(CozmoVision1Changed);
		}

		if(toggle_cozmoVisions.Length > 1 && toggle_cozmoVisions[1] != null && toggle_cozmoVisions[1].gameObject.activeSelf) {
			toggle_cozmoVisions[1].onValueChanged.AddListener(CozmoVision2Changed);
		}

		if(toggle_cozmoVisions.Length > 2 && toggle_cozmoVisions[2] != null && toggle_cozmoVisions[2].gameObject.activeSelf) {
			toggle_cozmoVisions[2].onValueChanged.AddListener(CozmoVision3Changed);
		}
	}

	void Init () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.value = PlayerPrefs.GetFloat("MaxTurnFactor", DEFAULT_MAX_TURN_FACTOR);
		}

		for(int i = 0; i < toggle_cozmoVisions.Length; ++i) {
			if(toggle_cozmoVisions[i] != null) {
				toggle_cozmoVisions[i].isOn = PlayerPrefs.GetInt("CozmoVision" + (i + 1), DEFAULT_COZMO_VISION) == 1;
			}
		}
	}

	void OnDisable () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) 
			slider_turnSpeed.onValueChanged.RemoveListener(MaxTurnSpeedChanged);

		if(toggle_cozmoVisions.Length > 0 && toggle_cozmoVisions[0] != null && toggle_cozmoVisions[0].gameObject.activeSelf) {
			toggle_cozmoVisions[0].onValueChanged.RemoveListener(CozmoVision1Changed);
		}
		
		if(toggle_cozmoVisions.Length > 1 && toggle_cozmoVisions[1] != null && toggle_cozmoVisions[1].gameObject.activeSelf) {
			toggle_cozmoVisions[1].onValueChanged.RemoveListener(CozmoVision2Changed);
		}
		
		if(toggle_cozmoVisions.Length > 2 && toggle_cozmoVisions[2] != null && toggle_cozmoVisions[2].gameObject.activeSelf) {
			toggle_cozmoVisions[2].onValueChanged.RemoveListener(CozmoVision3Changed);
		}
	}

	public void MaxTurnSpeedChanged (float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}

	public void CozmoVision1Changed (bool val) {
		if(val) {
			PlayerPrefs.SetInt("CozmoVision1", 1);
		} else {
			PlayerPrefs.SetInt("CozmoVision1", 0);
		}
	}

	public void CozmoVision2Changed (bool val) {
		if(val) {
			PlayerPrefs.SetInt("CozmoVision2", 1);
		} else {
			PlayerPrefs.SetInt("CozmoVision2", 0);
		}
	}

	public void CozmoVision3Changed (bool val) {
		if(val) {
			PlayerPrefs.SetInt("CozmoVision3", 1);
		} else {
			PlayerPrefs.SetInt("CozmoVision3", 0);
		}
	}

	public void ResetToDefaultSettings () {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		for(int i = 0; i < toggle_cozmoVisions.Length; ++i) {
			PlayerPrefs.DeleteKey("CozmoVision" + (i + 1));
		}
		Init();
	}
}
