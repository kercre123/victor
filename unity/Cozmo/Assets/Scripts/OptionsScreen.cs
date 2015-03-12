using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle[] toggle_cozmoVisions;
	[SerializeField] Toggle toggle_reverseLikeACar;
	[SerializeField] ComboBox combo_controls;
	[SerializeField] ComboBox combo_vision;

	public const int REVERSE_LIKE_A_CAR = 1;
	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;

	public string[] controlStyles = { "GyroSliderHybrid", "SliderAndTilt", "TwoSliders", "TriThumb", "DriveThumb", "PlayerThumb" };
	public string[] visionStyles = { "CozmoVision1", "CozmoVision2", "CozmoVision3" };

	void OnEnable () {
		Init();
		AddListeners();
	}

	void Init () {
		if(slider_turnSpeed != null && slider_turnSpeed.gameObject.activeSelf) {
			slider_turnSpeed.value = PlayerPrefs.GetFloat("MaxTurnFactor", DEFAULT_MAX_TURN_FACTOR);
		}

		if(toggle_reverseLikeACar != null) {
			toggle_reverseLikeACar.isOn = PlayerPrefs.GetInt("ReverseLikeACar", REVERSE_LIKE_A_CAR) == 1;
			//Debug.Log("Init toggle_reverseLikeACar.isOn("+toggle_reverseLikeACar.isOn+")");
		}

		if(combo_controls != null) {
			combo_controls.AddItems(controlStyles);
			combo_controls.OnSelectionChanged = ControlsSelected;
			combo_controls.SelectedIndex = PlayerPrefs.GetInt("ControlSchemeIndex", 0);
		}

		if(combo_vision != null) {
			combo_vision.AddItems(visionStyles);
			combo_vision.OnSelectionChanged = VisionSelected;
			combo_vision.SelectedIndex = PlayerPrefs.GetInt("VisionSchemeIndex", 0);
		}

	}

	void ControlsSelected(int index) {
		PlayerPrefs.SetInt("ControlSchemeIndex", index);
	}

	void VisionSelected(int index) {
		PlayerPrefs.SetInt("VisionSchemeIndex", index);
	}

	void AddListeners() {
		
		if(slider_turnSpeed != null) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}

		if(toggle_reverseLikeACar != null) {
			toggle_reverseLikeACar.onValueChanged.AddListener(ToggleReverseLikeACar);
		}
	}

	void OnDisable () {
		RemoveListeners();
	}

	void RemoveListeners() {
		if(slider_turnSpeed != null) {
			slider_turnSpeed.onValueChanged.RemoveListener(MaxTurnSpeedChanged);
		}

		if(toggle_reverseLikeACar != null) {
			toggle_reverseLikeACar.onValueChanged.RemoveListener(ToggleReverseLikeACar);
		}
	}

	public void MaxTurnSpeedChanged (float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}

	public void ToggleReverseLikeACar (bool val) {
		if(val) {
			PlayerPrefs.SetInt("ReverseLikeACar", 1);
		} else {
			PlayerPrefs.SetInt("ReverseLikeACar", 0);
		}

		//Debug.Log("ToggleReverseLikeACar("+val+") PlayerPrefs.SetInt(ReverseLikeACar, "+PlayerPrefs.GetInt("ReverseLikeACar", REVERSE_LIKE_A_CAR)+")");
	}

	public void ResetToDefaultSettings () {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		for(int i = 0; i < toggle_cozmoVisions.Length; ++i) {
			PlayerPrefs.DeleteKey("CozmoVision" + (i + 1));
		}
		PlayerPrefs.DeleteKey("ReverseLikeACar");

		RemoveListeners();
		Init();
		AddListeners();
	}
}
