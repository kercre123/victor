using UnityEngine;
using UnityEngine.UI;
using System;
using System.Collections;

public class OptionsScreen : MonoBehaviour {

	[SerializeField] GameObject optionsAnchor;
	[SerializeField] Slider slider_turnSpeed;
	[SerializeField] Toggle toggle_reverseLikeACar;
	[SerializeField] ComboBox combo_controls;
	[SerializeField] ComboBox combo_vision;
	[SerializeField] Toggle toggle_debug;
	[SerializeField] Toggle toggle_flushLogs;
	[SerializeField] Toggle toggle_vision;
	[SerializeField] Toggle toggle_visionFade;
	[SerializeField] Toggle toggle_visionRecording;
	[SerializeField] Toggle toggle_userTestMode;
	[SerializeField] Toggle toggle_autoCollect;
	[SerializeField] Toggle toggle_assistedControls;
	[SerializeField] Toggle toggle_skipLayoutTracker;

	[SerializeField] ComboBox pertinent_objects;
	[SerializeField] InputField angleScoreMultiplier;
	[SerializeField] InputField distanceScoreMultiplier;
	[SerializeField] InputField maxAngle;
	[SerializeField] InputField maxDistanceInCubeLengths;

	[SerializeField] InputField spinnerSettings_StartMultiplier;
	[SerializeField] InputField spinnerSettings_MinStart;
	[SerializeField] InputField spinnerSettings_MaxSpeed;
	[SerializeField] InputField spinnerSettings_MinSpeed;
	[SerializeField] InputField spinnerSettings_Drag;
	[SerializeField] InputField spinnerSettings_MaxSamples;
	[SerializeField] InputField spinnerSettings_PegThreshold;
	[SerializeField] InputField spinnerSettings_PegSlowFactor;


	public static Action RefreshSettings = null;

	public const int REVERSE_LIKE_A_CAR = 0;
	public const float DEFAULT_MAX_TURN_FACTOR = 0.25f;

	public string[] controlStyles = { "GyroSliderHybrid", "SliderAndTilt", "TwoSliders", "TriThumb", "DriveThumb", "PlayerThumb" };
	public string[] visionStyles = { "CozmoVision2", "CozmoVision3", "CozmoVision4" };
	public string[] pertinentObjectTypes = { 
		Robot.ObservedObjectListType.OBSERVED_RECENTLY.ToString(),
		Robot.ObservedObjectListType.MARKERS_SEEN.ToString(),
		Robot.ObservedObjectListType.KNOWN.ToString() };

	public static OptionsScreen instance = null;
	public static bool IsOpen {
		get {
			if(instance == null) return false;
			if(instance.optionsAnchor == null) return false;
			return instance.optionsAnchor.activeSelf;
		}
	}

	Robot robot { get { return RobotEngineManager.instance != null ? RobotEngineManager.instance.current : null; } }

	void Awake () {
		if(instance != null) {
			//Debug.Log("OptionsScreen destroying old options, because scene specific layouts might differ.");
			GameObject.Destroy(instance.gameObject);
			//return;
		}
		instance = this;
		DontDestroyOnLoad(gameObject);
	}

	void OnEnable () {
		if(instance != this) return;

		Init();
		AddListeners();
		optionsAnchor.SetActive(false);
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

		if(toggle_flushLogs != null) {
			toggle_flushLogs.isOn = PlayerPrefs.GetInt("FlushLogs", 1) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_vision != null) {
			toggle_vision.isOn = PlayerPrefs.GetInt("VisionDisabled" + GetVisionSelected().ToString(), 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_visionFade != null) {
			toggle_visionFade.isOn = PlayerPrefs.GetInt("VisionFadeDisabled" + GetVisionSelected().ToString(), 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_userTestMode != null) {
			toggle_userTestMode.isOn = PlayerPrefs.GetInt("UserTestMode", 0) == 1;
			//Debug.Log("Init toggle_debug.isOn("+toggle_debug.isOn+")");
		}

		if(toggle_autoCollect != null) {
			toggle_autoCollect.isOn = PlayerPrefs.GetInt("EnergyHuntAutoCollect", 0) == 1;
		}

		if(toggle_assistedControls != null) {
			toggle_assistedControls.isOn = PlayerPrefs.GetInt("CozmoAssistedControls", 0) == 1;
		}

		if(toggle_skipLayoutTracker != null) {
			toggle_skipLayoutTracker.isOn = PlayerPrefs.GetInt("DebugSkipLayoutTracker",0) == 1;
		}

		if(toggle_visionRecording != null) {
			toggle_visionRecording.isOn = RobotEngineManager.instance != null ? RobotEngineManager.instance.AllowImageSaving : false;
		}

		if(combo_controls != null) {
			combo_controls.InitControl();
			combo_controls.ClearItems();
			combo_controls.AddItems(controlStyles);
			combo_controls.OnSelectionChanged = ControlsSelected;
			combo_controls.SelectedIndex = PlayerPrefs.GetInt("ControlSchemeIndex", 0);
		}

		if(combo_vision != null) {
			combo_vision.InitControl();
			combo_vision.ClearItems();
			combo_vision.AddItems(visionStyles);
			combo_vision.OnSelectionChanged = SetVisionSelected;
			combo_vision.SelectedIndex = PlayerPrefs.GetInt("VisionSchemeIndex", 0);
		}

		if(pertinent_objects != null) {
			pertinent_objects.InitControl();
			pertinent_objects.ClearItems();
			pertinent_objects.AddItems(pertinentObjectTypes);
			pertinent_objects.OnSelectionChanged = ObjectPertinence;
			pertinent_objects.SelectedIndex = GetObjectPertinenceTypeOverride();
		}

		if(angleScoreMultiplier != null) {
			angleScoreMultiplier.text = GetAngleScoreMultiplier().ToString();
			angleScoreMultiplier.onValueChange.AddListener(AngleScoreMultiplier);
		}

		if(distanceScoreMultiplier != null) {
			distanceScoreMultiplier.text = GetDistanceScoreMultiplier().ToString();
			distanceScoreMultiplier.onValueChange.AddListener(DistanceScoreMultiplier);
		}

		if(maxAngle != null) {
			maxAngle.text = GetMaxAngle().ToString();
			maxAngle.onValueChange.AddListener(MaxAngle);
		}

		if(maxDistanceInCubeLengths != null) {
			maxDistanceInCubeLengths.text = GetMaxDistanceInCubeLengths().ToString();
			maxDistanceInCubeLengths.onValueChange.AddListener(MaxDistanceInCubeLengths);
		}

		spinnerSettings_StartMultiplier.text = PlayerPrefs.GetFloat("SpinMultiplier", 2f).ToString();
		spinnerSettings_StartMultiplier.onValueChange.AddListener(SetSpinnerSetting_StartMultiplier);

		spinnerSettings_MinStart.text = PlayerPrefs.GetFloat("SpinnerMinStartingVelocityDegPS", 360f).ToString();
		spinnerSettings_MinStart.onValueChange.AddListener(SetSpinnerSetting_MinStart);

		spinnerSettings_MaxSpeed.text = PlayerPrefs.GetFloat("SpinnerMaxAngularVelocityDegPS", 3600f).ToString();
		spinnerSettings_MaxSpeed.onValueChange.AddListener(SetSpinnerSetting_MaxSpeed);

		spinnerSettings_MinSpeed.text = PlayerPrefs.GetFloat("SpinnerMinAngularVelocityDegPS", 1f).ToString();
		spinnerSettings_MinSpeed.onValueChange.AddListener(SetSpinnerSetting_MinSpeed);

		spinnerSettings_Drag.text = PlayerPrefs.GetFloat("DragCoefficient", 0.3f).ToString();
		spinnerSettings_Drag.onValueChange.AddListener(SetSpinnerSetting_Drag);

		spinnerSettings_MaxSamples.text = PlayerPrefs.GetInt("MaxDragSamples", 5).ToString();
		spinnerSettings_MaxSamples.onValueChange.AddListener(SetSpinnerSetting_MaxSamples);

		spinnerSettings_PegThreshold.text = PlayerPrefs.GetFloat("SpinnerPegsSlowDownThreshold", 360f).ToString();
		spinnerSettings_PegThreshold.onValueChange.AddListener(SetSpinnerSetting_PegThreshold);

		spinnerSettings_PegSlowFactor.text = PlayerPrefs.GetFloat("SpinnerPegsSlowDownFactor", 0.9f).ToString();
		spinnerSettings_PegSlowFactor.onValueChange.AddListener(SetSpinnerSetting_PegSlowFactor);
	}

	void SetSpinnerSetting_StartMultiplier(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinMultiplier", value);
	}
	void SetSpinnerSetting_MinStart(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinnerMinStartingVelocityDegPS", value);
	}
	void SetSpinnerSetting_MaxSpeed(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinnerMaxAngularVelocityDegPS", value);
	}
	void SetSpinnerSetting_MinSpeed(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinnerMinAngularVelocityDegPS", value);
	}
	void SetSpinnerSetting_Drag(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("DragCoefficient", value);
	}
	void SetSpinnerSetting_MaxSamples(string text) {
		int value;
		if(!int.TryParse(text, out value)) return;
		PlayerPrefs.SetInt("MaxDragSamples", value);
	}
	void SetSpinnerSetting_PegThreshold(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinnerPegsSlowDownThreshold", value);
	}
	void SetSpinnerSetting_PegSlowFactor(string text) {
		float value;
		if(!float.TryParse(text, out value)) return;
		PlayerPrefs.SetFloat("SpinnerPegsSlowDownFactor", value);
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
		
		if(toggle_flushLogs != null) {
			toggle_flushLogs.onValueChanged.AddListener(ToggleFlushLogs);
		}

		if(toggle_vision != null) {
			toggle_vision.onValueChanged.AddListener(ToggleDisableVision);
		}

		if(toggle_visionFade != null) {
			toggle_visionFade.onValueChanged.AddListener(ToggleDisableVisionFade);
		}

		if(toggle_visionRecording != null) {
			toggle_visionRecording.onValueChanged.AddListener(ToggleVisionRecording);
		}

		if(toggle_userTestMode != null) {
			toggle_userTestMode.onValueChanged.AddListener(ToggleUserTestMode);
		}

		if(toggle_autoCollect != null) {
			toggle_autoCollect.onValueChanged.AddListener(ToggleEnergyHuntAutoCollect);
		}
		if(toggle_assistedControls != null) {
			toggle_assistedControls.onValueChanged.AddListener(ToggleAssitedControls);
		}
		if(toggle_skipLayoutTracker != null) {
			toggle_skipLayoutTracker.onValueChanged.AddListener(ToggleSkipLayoutTracker);
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

		if(toggle_visionFade != null) {
			toggle_visionFade.onValueChanged.RemoveListener(ToggleDisableVisionFade);
		}
	}
	
	void ControlsSelected(int index) {
		PlayerPrefs.SetInt("ControlSchemeIndex", index);
	}

	static int GetVisionSelected(int defaultValue = 0) {
		return PlayerPrefs.GetInt("VisionSchemeIndex", defaultValue);
	}

	void SetVisionSelected(int index) {
		PlayerPrefs.SetInt("VisionSchemeIndex", index);
		Init();
	}

	void ObjectPertinence(int value) {
		PlayerPrefs.SetInt("ObjectPertinence" + GetVisionSelected().ToString(), value);
		if(robot != null) robot.RefreshObjectPertinence();
	}

	void AngleScoreMultiplier(string value) {
		float multiplier;
		if(float.TryParse(value, out multiplier)) {
			PlayerPrefs.SetFloat("AngleScoreMultiplier" + GetVisionSelected().ToString(), multiplier);
		}
		
		ObservedObject.RefreshAngleScoreMultiplier();
	}

	void DistanceScoreMultiplier(string value) {
		float multiplier;
		if(float.TryParse(value, out multiplier)) {
			PlayerPrefs.SetFloat("DistanceScoreMultiplier" + GetVisionSelected().ToString(), multiplier);
		}
		
		ObservedObject.RefreshDistanceScoreMultiplier();
	}

	void MaxAngle(string value) {
		float ignore;
		if(float.TryParse(value, out ignore)) {
			PlayerPrefs.SetFloat("MaxAngle" + GetVisionSelected().ToString(), ignore);
		}
		
		ObservedObject.RefreshMaxAngle();
	}

	void MaxDistanceInCubeLengths(string value) {
		float ignore;
		if(float.TryParse(value, out ignore)) {
			PlayerPrefs.SetFloat("MaxDistanceInCubeLengths" + GetVisionSelected().ToString(), ignore);
		}
		
		ObservedObject.RefreshMaxDistanceInCubeLengths();
		if(robot != null) robot.RefreshObjectPertinence();
	}

	void MaxTurnSpeedChanged(float val) {
		PlayerPrefs.SetFloat("MaxTurnFactor", Mathf.Clamp01(val));
	}
	
	void ToggleReverseLikeACar(bool val) {
		PlayerPrefs.SetInt("ReverseLikeACar", val ? 1 : 0);
	}

	void ToggleShowDebugInfo(bool val) {
		PlayerPrefs.SetInt("ShowDebugInfo", val ? 1 : 0);
	}

	void ToggleFlushLogs(bool val) {
		PlayerPrefs.SetInt("FlushLogs", val ? 1 : 0);
	}

	void ToggleDisableVision(bool val) {
		PlayerPrefs.SetInt("VisionDisabled" + GetVisionSelected().ToString(), val ? 1 : 0);
	}

	void ToggleDisableVisionFade(bool val) {
		PlayerPrefs.SetInt("VisionFadeDisabled" + GetVisionSelected().ToString(), val ? 1 : 0);
	}

	public static bool GetToggleDisableVision(bool defaultValue = false) {
		return PlayerPrefs.GetInt("VisionDisabled" + GetVisionSelected().ToString(), defaultValue ? 1 : 0) > 0 ? true : false;
	}

	public static bool GetToggleDisableVisionFade(bool defaultValue = false) {
		return PlayerPrefs.GetInt("VisionFadeDisabled" + GetVisionSelected().ToString(), defaultValue ? 1 : 0) > 0 ? true : false;
	}

	public static int GetObjectPertinenceTypeOverride(int defaultValue = 0) {
		return PlayerPrefs.GetInt("ObjectPertinence" + GetVisionSelected().ToString(), defaultValue);
	}

	public static float GetAngleScoreMultiplier(float defaultValue = 1f) {
		return PlayerPrefs.GetFloat("AngleScoreMultiplier" + GetVisionSelected().ToString(), defaultValue);
	}

	public static float GetDistanceScoreMultiplier(float defaultValue = 1f) {
		return PlayerPrefs.GetFloat("DistanceScoreMultiplier" + GetVisionSelected().ToString(), defaultValue);
	}

	public static float GetMaxAngle(float defaultValue = 45f) {
		return PlayerPrefs.GetFloat("MaxAngle" + GetVisionSelected().ToString(), defaultValue);
	}

	public static float GetMaxDistanceInCubeLengths(float defaultValue = 8f) {
		return PlayerPrefs.GetFloat("MaxDistanceInCubeLengths" + GetVisionSelected().ToString(), defaultValue);
	}

	void ToggleUserTestMode(bool val) {
		PlayerPrefs.SetInt("UserTestMode", val ? 1 : 0);
		//Debug.Log("ToggleUserTestMode("+val+")");
	}

	void ToggleVisionRecording(bool val) {
		if(RobotEngineManager.instance != null) RobotEngineManager.instance.ToggleVisionRecording(val);
	}
	
	void ToggleEnergyHuntAutoCollect(bool val) {
		PlayerPrefs.SetInt("EnergyHuntAutoCollect", val ? 1 : 0);
		//Debug.Log("ToggleUserTestMode("+val+")");
	}

	void ToggleAssitedControls(bool val) {
		PlayerPrefs.SetInt("CozmoAssistedControls", val ? 1 : 0);
	}

	void ToggleSkipLayoutTracker(bool val) {
		PlayerPrefs.SetInt("DebugSkipLayoutTracker", val ? 1 : 0);
	}

	public void ResetToDefaultSettings() {
		PlayerPrefs.DeleteKey("MaxTurnFactor");
		PlayerPrefs.DeleteKey("ReverseLikeACar");
		PlayerPrefs.DeleteKey("ControlSchemeIndex");
		PlayerPrefs.DeleteKey("VisionSchemeIndex");
		PlayerPrefs.DeleteKey("ShowDebugInfo");
		PlayerPrefs.DeleteKey("ToggleUserTestMode");
		PlayerPrefs.DeleteKey("EnergyHuntAutoCollect");
		PlayerPrefs.DeleteKey("FlushLogs");
		PlayerPrefs.DeleteKey("CozmoAssistedControls");
		PlayerPrefs.DeleteKey("DebugSkipLayoutTracker");


		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("ObjectPertinence" + i.ToString());
		}

		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("VisionDisabled" + i.ToString());
		}

		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("VisionFadeDisabled" + i.ToString());
		}

		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("AngleScoreMultiplier" + i.ToString());
		}

		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("DistanceScoreMultiplier" + i.ToString());
		}

		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("MaxAngle" + i.ToString());
		}
		for(int i = 0; i < 5; ++i) {
			PlayerPrefs.DeleteKey("MaxDistanceInCubeLengths" + i.ToString());
		}

		PlayerPrefs.DeleteKey("SpinMultiplier");
		PlayerPrefs.DeleteKey("SpinnerMinStartingVelocityDegPS");
		PlayerPrefs.DeleteKey("SpinnerMaxAngularVelocityDegPS");
		PlayerPrefs.DeleteKey("SpinnerMinAngularVelocityDegPS");
		PlayerPrefs.DeleteKey("DragCoefficient");
		PlayerPrefs.DeleteKey("MaxDragSamples");
		PlayerPrefs.DeleteKey("SpinnerPegsSlowDownThreshold");
		PlayerPrefs.DeleteKey("SpinnerPegsSlowDownFactor");

		RemoveListeners();
		Init();
		AddListeners();
	}

	public void Toggle(bool toggle) {
		optionsAnchor.SetActive(toggle);

		if(!toggle && RefreshSettings != null) RefreshSettings();
	}
}
