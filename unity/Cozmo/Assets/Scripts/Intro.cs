using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
	[SerializeField] protected InputField id;
	[SerializeField] protected InputField ip;
	[SerializeField] protected InputField visualizerIP;
	[SerializeField] protected Toggle[] schemeToggles;
	[SerializeField] protected Toggle[] orientationToggles;
	[SerializeField] protected Toggle simulated;
	[SerializeField] protected Button play;
	[SerializeField] protected Text error;

	private bool connecting = false;
	private float hackWait = 0.0f;

	private string[] scenes = { "ThumbStick", "ScreenPad", "TwoSliders" };
	private ScreenOrientation[] orientations = { ScreenOrientation.Portrait, ScreenOrientation.PortraitUpsideDown, ScreenOrientation.LandscapeLeft, ScreenOrientation.LandscapeRight };

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

	private string lastVisualizerIp
	{
		get { return PlayerPrefs.GetString("LastVisualizerIp", "127.0.0.1"); }
		
		set { PlayerPrefs.SetString("LastVisualizerIp", value); }
	}

	private int lastSceneIndex
	{
		get { 
			int index = PlayerPrefs.GetInt("LastSceneIndex", 0);
			index = Mathf.Clamp(index, 0, scenes.Length-1);
			return index; }
		
		set { PlayerPrefs.SetInt("LastSceneIndex", value); }
	}

	protected void OnEnable() {
		ip.text = lastIp;
		id.text = lastId;
		simulated.isOn = lastSimulated;

		visualizerIP.text = lastVisualizerIp;

		//initialize scheme choices
		for(int i=0;i<schemeToggles.Length;i++) {
			schemeToggles[i].isOn = i == lastSceneIndex;
		}

		for(int i=0;i<schemeToggles.Length;i++) {
			schemeToggles[i].onValueChanged.AddListener(RefreshScheme);
		}

		//initialize orientation choices
		ScreenOrientation orientation = Screen.orientation;
		if(orientation == ScreenOrientation.AutoRotation || orientation == ScreenOrientation.Unknown) {
			orientation = ScreenOrientation.LandscapeLeft;
			if(!Application.isEditor) Screen.orientation = orientation;

		}

		for(int i=0;i<orientationToggles.Length;i++) {
			orientationToggles[i].isOn = orientation == orientations[i];
		}

		for(int i=0;i<orientationToggles.Length;i++) {
			orientationToggles[i].onValueChanged.AddListener(RefreshOrientation);
		}

	}

	public void RefreshScheme(bool on) {
		if(!on || Application.isEditor) return;
		
		int index = 0;
		for(int i=0; i<schemeToggles.Length; i++) {
			if(!schemeToggles[i].isOn) continue;
			
			index = i;
			break;
		}

		lastSceneIndex = index;
		Debug.Log("RefreshScheme("+index+")");
	}

	public void RefreshOrientation(bool on) {
		if(!on || Application.isEditor) return;

		int index = 0;
		for(int i=0; i<orientationToggles.Length; i++) {
			if(!orientationToggles[i].isOn) continue;

			index = i;
			break;
		}

		index = Mathf.Clamp(index, 0, orientations.Length - 1);
		ScreenOrientation orientation = (ScreenOrientation)orientations[index];
		Screen.orientation = orientation;
	}

	protected void Update() {
		if(connecting && Time.time > hackWait) {
			if(RobotEngineManager.instance.IsRobotConnected(CurrentRobotID)) {
				connecting = false;
				error.text = "";
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
			errorText = "You must enter a robot ip address.";
		}

		if(string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(visualizerIP.text)) {
			errorText = "You must enter a visualizer ip address.";
		}

		if(string.IsNullOrEmpty(errorText)) {

			bool hostCreated = RobotEngineManager.instance.CreateHostWithVisIP(visualizerIP.text);

			if(hostCreated) {
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
		}

		error.text = errorText;
	}

	protected void SaveData() {
		lastIp = ip.text;
		lastId = id.text;
		lastSimulated = simulated.isOn;
		lastVisualizerIp = visualizerIP.text;

		for(int i=0;i<schemeToggles.Length;i++) {
			if(!schemeToggles[i].isOn) continue;
			lastSceneIndex = i;
			break;
		}

	}

	public void FakeTest() {
		SaveData();
		Application.LoadLevel(scenes[lastSceneIndex]);
	}

}
