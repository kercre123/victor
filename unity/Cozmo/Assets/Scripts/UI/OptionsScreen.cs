using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle toggle_reverseLikeACar;
	[SerializeField] ComboBox combo_controls;
	[SerializeField] ComboBox combo_vision;
	[SerializeField] Toggle toggle_debug;
	[SerializeField] Toggle toggle_vision;
	[SerializeField] Toggle toggle_visionRecording;
	[SerializeField] Toggle toggle_userTestMode;

	[SerializeField] ComboBox pertinent_objects;

	public const int REVERSE_LIKE_A_CAR = 1;
	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;

	public string[] controlStyles = { "GyroSliderHybrid", "SliderAndTilt", "TwoSliders", "TriThumb", "DriveThumb", "PlayerThumb" };
	public string[] visionStyles = { "CozmoVision2", "CozmoVision3", "CozmoVision4" };
	public string[] pertinentObjectTypes = { 
		CozmoVision.ObservedObjectListType.OBSERVED_RECENTLY.ToString(),
		CozmoVision.ObservedObjectListType.MARKERS_SEEN.ToString(),
		CozmoVision.ObservedObjectListType.KNOWN.ToString() };

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

		if(toggle_debug != null) {
			toggle_debug.isOn = PlayerPrefs.GetInt("ShowDebugInfo", 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_vision != null) {
			toggle_vision.isOn = PlayerPrefs.GetInt("VisionDisabled", 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_userTestMode != null) {
			toggle_userTestMode.isOn = PlayerPrefs.GetInt("UserTestMode", 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_visionRecording != null) {
			toggle_visionRecording.isOn = RobotEngineManager.instance != null ? RobotEngineManager.instance.AllowImageSaving : false;
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

		if(pertinent_objects != null) {
			pertinent_objects.AddItems(pertinentObjectTypes);
			pertinent_objects.OnSelectionChanged = ObjectPertinence;
			pertinent_objects.SelectedIndex = PlayerPrefs.GetInt("ObjectPertinence", -1);
		}

	}

	void AddListeners() {
		
		if(slider_turnSpeed != null) {
			slider_turnSpeed.onValueChanged.AddListener(MaxTurnSpeedChanged);
		}

		if(toggle_reverseLikeACar != null) {
			toggle_reverseLikeACar.onValueChanged.AddListener(ToggleReverseLikeACar);
		}

		if(toggle_debug != null) {
			toggle_debug.onValueChanged.AddListener(ToggleShowDebugInfo);
		}

		if(toggle_vision != null) {
			toggle_vision.onValueChanged.AddListener(ToggleDisableVision);
		}

		if(toggle_visionRecording != null) {
			toggle_visionRecording.onValueChanged.AddListener(ToggleVisionRecording);
		}

		if(toggle_userTestMode != null) {
			toggle_userTestMode.onValueChanged.AddListener(ToggleUserTestMode);
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

		if(toggle_debug != null) {
			toggle_debug.onValueChanged.RemoveListener(ToggleShowDebugInfo);
		}

		if(toggle_vision != null) {
			toggle_vision.onValueChanged.RemoveListener(ToggleDisableVision);
		}
	}

	void ControlsSelected(int index) {
		PlayerPrefs.SetInt("ControlSchemeIndex", index);
	}
	
	void VisionSelected(int index) {
		PlayerPrefs.SetInt("VisionSchemeIndex", index);
	}

	void ObjectPertinence(int index) {
		PlayerPrefs.SetInt("ObjectPertinence", index);
	}

	public void MaxTurnSpeedChanged (float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}

	public void ToggleReverseLikeACar (bool val) {
		PlayerPrefs.SetInt("ReverseLikeACar", val ? 1 : 0);
	}

	public void ToggleShowDebugInfo (bool val) {
		PlayerPrefs.SetInt("ShowDebugInfo", val ? 1 : 0);
	}

	public void ToggleDisableVision (bool val) {
		PlayerPrefs.SetInt("VisionDisabled", val ? 1 : 0);
	}

	public void ToggleUserTestMode (bool val) {
		PlayerPrefs.SetInt("UserTestMode", val ? 1 : 0);
		Debug.Log("ToggleUserTestMode("+val+")");
	}

	public void ToggleVisionRecording (bool val) {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.ToggleVisionRecording(val);
	}

	public void ResetToDefaultSettings () {

		PlayerPrefs.DeleteKey("MaxTurnFactor");
		PlayerPrefs.DeleteKey("ReverseLikeACar");
		PlayerPrefs.DeleteKey("ControlSchemeIndex");
		PlayerPrefs.DeleteKey("VisionSchemeIndex");
		PlayerPrefs.DeleteKey("VisionDisabled");
		PlayerPrefs.DeleteKey("ShowDebugInfo");

		RemoveListeners();
		Init();
		AddListeners();
	}
}
