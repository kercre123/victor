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

	protected void Awake() {
		ip.text = lastIp;
		id.text = lastId;
		simulated.isOn = lastSimulated;

		visualizerIP.text = lastVisualizerIp;

		//initialize scheme choices
		for(int i=0;i<schemeToggles.Length;i++) {
			schemeToggles[i].isOn = i == lastSceneIndex;
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
	}

	public void ChooseScheme(int choice) {
		lastSceneIndex = choice;
		Debug.Log("ChooseScheme("+choice+")");
	}

	public void ChooseOrientation(int choice) {
		if(Application.isEditor)
			return;
		choice = Mathf.Clamp(choice, 0, orientations.Length - 1);
		ScreenOrientation orientation = (ScreenOrientation)choice;
		Screen.orientation = orientation;
	}

	protected void Start()
	{
		RobotEngineManager.instance.ConnectedToClient += Connected;
		RobotEngineManager.instance.DisconnectedFromClient += Disconnected;
	}

	public void Play() {
		RobotEngineManager.instance.Disconnect ();

		CurrentRobotID = 0;

		string errorText = null;
		int idInteger;
		if(!int.TryParse(id.text, out idInteger) || idInteger == 0) {
			errorText = "You must enter a nonzero id.";
		}
		if(string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ip.text)) {
			errorText = "You must enter a robot ip address.";
		}
        error.text = errorText;

		if(string.IsNullOrEmpty(errorText)) {
			SaveData();
			CurrentRobotID = idInteger;
			RobotEngineManager.instance.Connect (ip.text);
		}
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

	private void Connected(string connectionIdentifier)
	{
		error.text = "Connected to " + connectionIdentifier + ". Force-adding robot...";
		RobotEngineManager.instance.ForceAddRobot(CurrentRobotID, ip.text, simulated.isOn);
	}

	private void Disconnected(DisconnectionReason reason)
	{
		error.text = "Disconnected: " + reason.ToString ();
	}

}
