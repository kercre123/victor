using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
	[SerializeField] protected InputField id;
	[SerializeField] protected InputField ip;
	[SerializeField] protected Text schemeLabel;
	[SerializeField] protected Slider schemeSlider;
	[SerializeField] protected Slider orientationSlider;
	[SerializeField] protected Text orientationLabel;
	[SerializeField] protected Toggle simulated;
	[SerializeField] protected Button play;
	[SerializeField] protected Text error;

	private bool connecting = false;
	private float hackWait = 0.0f;

	private string[] scenes = { "ThumbStick", "ScreenPad", "TwoSliders" };

	public static int CurrentRobotID { get; private set; }

	private string lastIp
	{
		get { return PlayerPrefs.GetString("LastIP", "127.0.0.1"); }

		set { PlayerPrefs.SetString("LastIP", value); }
	}

	private string lastId
	{
		get { return PlayerPrefs.GetString("LastID", "1"); }
		
		set { PlayerPrefs.SetString("LastID", value); }
	}

	private bool lastSimulated
	{
		get { return PlayerPrefs.GetInt("LastSimulated", 1) == 1; }
		
		set { if(value) { PlayerPrefs.SetInt("LastSimulated", 1); } else { PlayerPrefs.SetInt("LastSimulated", 0); } }
	}

	private int lastSceneIndex
	{
		get { 
			int index = PlayerPrefs.GetInt("LastSceneIndex", 0);
			index = Mathf.Clamp(index, 0, scenes.Length-1);
			return index; }
		
		set { PlayerPrefs.SetInt("LastSceneIndex", value); }
	}

	protected void Awake() {
		ip.text = lastIp;
		id.text = lastId;
		simulated.isOn = lastSimulated;
		schemeSlider.wholeNumbers = true;
		schemeSlider.minValue = 0;
		schemeSlider.maxValue = scenes.Length-1;
		schemeSlider.value = lastSceneIndex;
		schemeSlider.onValueChanged.AddListener(SchemeChanged);
		schemeLabel.text = "ControlScheme: " + scenes[lastSceneIndex];
		
		orientationSlider.wholeNumbers = true;
		orientationSlider.minValue = (int)ScreenOrientation.Portrait;
		orientationSlider.maxValue = (int)ScreenOrientation.LandscapeRight;

		ScreenOrientation orientation = Screen.orientation;
		if(orientation == ScreenOrientation.Unknown || orientation == ScreenOrientation.AutoRotation) {
			orientation = ScreenOrientation.LandscapeLeft;
			Screen.orientation = orientation;
		}

		orientationSlider.value = (int)Screen.orientation;
		orientationLabel.text = Screen.orientation.ToString();
		orientationSlider.onValueChanged.AddListener(OrientationChanged);
	}

	private void SchemeChanged(float val) {
		lastSceneIndex = (int)val;
		schemeLabel.text = "ControlScheme: " + scenes[lastSceneIndex];
	}

	public void OrientationChanged(float val) {
		ScreenOrientation orientation = (ScreenOrientation)val;
		if(orientation >= ScreenOrientation.AutoRotation)
			orientation = ScreenOrientation.Portrait;
		Screen.orientation = orientation;
		orientationLabel.text = orientation.ToString();
	}

	protected void Update() {
		if(connecting && Time.time > hackWait) {
			if(RobotEngineManager.instance.IsRobotConnected(CurrentRobotID)) {
				connecting = false;
				error.text = "";
				//RobotEngineManager.instance.DriveWheels(CurrentRobotID, 50.0f, 50.0f);

				Application.LoadLevel(scenes[lastSceneIndex]);
			}
		}
	}

	public void Play() {
		CurrentRobotID = 0;

		string errorText = null;
		int idInteger;
		if(!int.TryParse(id.text, out idInteger) || idInteger == 0) {
			errorText = "You must enter a nonzero id.";
		}
		if(string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ip.text)) {
			errorText = "You must enter an ip address.";
		}
		if(string.IsNullOrEmpty(errorText)) {
			CozmoResult result = RobotEngineManager.instance.ForceAddRobot(idInteger, ip.text, simulated.isOn);
			if(result != CozmoResult.OK) {
				errorText = "Error attempting to add robot: " + result.ToString();
			}
			else {
				connecting = true;
				SaveData();
				CurrentRobotID = idInteger;
				hackWait = Time.time + 5.0f;
				errorText = "Connecting...";
			}
		}

		error.text = errorText;
	}

	protected void SaveData()
	{
		lastIp = ip.text;
		lastId = id.text;
		lastSimulated = simulated.isOn;
		lastSceneIndex = (int)schemeSlider.value;
	}

	public void FakeTest() {
		SaveData();
		Application.LoadLevel(scenes[lastSceneIndex]);
	}

}
