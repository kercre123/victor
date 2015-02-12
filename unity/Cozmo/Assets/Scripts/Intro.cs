using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
	[SerializeField] protected InputField engineIP;
	[SerializeField] protected InputField ip;
	[SerializeField] protected InputField visualizerIP;
	[SerializeField] protected Toggle simulated;
	[SerializeField] protected Text orientationLabel;
	[SerializeField] protected Text error;

	private string currentRobotIP;
	private string currentScene;
	private string currentVizHostIP;

	private ScreenOrientation[] test_orientations = {
		ScreenOrientation.Portrait,
		ScreenOrientation.PortraitUpsideDown,
		ScreenOrientation.LandscapeLeft,
		ScreenOrientation.LandscapeRight
	};

	private int test_orientation_index = 2;

	public const int CurrentRobotID = 1;

	private string lastEngineIp
	{
		get { return PlayerPrefs.GetString("LastEngineIP", "127.0.0.1"); }

		set { PlayerPrefs.SetString("LastEngineIP", value); }
	}

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

	private string lastSceneName
	{
		get { 
			string sceneName = PlayerPrefs.GetString("LastSceneName", "ThumbStick");
			return sceneName;
		}
		
		set { PlayerPrefs.SetString("LastSceneName", value); }
	}

	protected void OnEnable() {
		engineIP.text = lastEngineIp;
		ip.text = lastIp;
		simulated.isOn = lastSimulated;
		visualizerIP.text = lastVisualizerIp;
		//force portrait in shell to ensure working on phones
		Screen.orientation = ScreenOrientation.Portrait;
		orientationLabel.text = "Orientation: " + test_orientations[test_orientation_index].ToString();
	}

	public void PlayScheme(string choice) {
		lastSceneName = choice;
		Play();
	}

	protected void Start()
	{
		if (RobotEngineManager.instance != null) {
			RobotEngineManager.instance.ConnectedToClient += Connected;
			RobotEngineManager.instance.DisconnectedFromClient += Disconnected;
			RobotEngineManager.instance.RobotConnected += RobotConnected;
		}
	}

	protected void OnDestroy() {
		if (RobotEngineManager.instance != null) {
			RobotEngineManager.instance.ConnectedToClient -= Connected;
			RobotEngineManager.instance.DisconnectedFromClient -= Disconnected;
			RobotEngineManager.instance.RobotConnected -= RobotConnected;
		}
	}

	public void Play() {
		RobotEngineManager.instance.Disconnect ();

		string errorText = null;

		if(string.IsNullOrEmpty(engineIP.text)) {
			errorText = "You must enter a device ip address.";
		}
		if(string.IsNullOrEmpty(errorText) && string.IsNullOrEmpty(ip.text)) {
			errorText = "You must enter a robot ip address.";
		}

		if (string.IsNullOrEmpty (errorText)) {
			currentRobotIP = ip.text;
			currentScene = lastSceneName;
			currentVizHostIP = visualizerIP.text;

			SaveData ();
			RobotEngineManager.instance.Connect (engineIP.text);
			error.text = "<color=#ffffff>Connecting to engine at " + ip.text + "....</color>";
		} else {
			error.text = errorText;
		}
	}

	protected void SaveData() {
		lastIp = ip.text;
		lastId = engineIP.text;
		lastSimulated = simulated.isOn;
		lastVisualizerIp = visualizerIP.text;
	}

	public void FakeTest() {
		SaveData();
		Application.LoadLevel(lastSceneName);
	}

	private void Connected(string connectionIdentifier)
	{
		error.text = "<color=#ffffff>Connected to " + connectionIdentifier + ". Force-adding robot...</color>";
		RobotEngineManager.instance.StartEngine (currentVizHostIP);
		RobotEngineManager.instance.ForceAddRobot(CurrentRobotID, currentRobotIP, simulated.isOn);
	}

	private void Disconnected(DisconnectionReason reason)
	{
		error.text = "Disconnected: " + reason.ToString ();
	}

	private void RobotConnected(int robotID)
	{
		if (CurrentRobotID != robotID) {
			Debug.LogError ("Unknown robot connected: " + robotID.ToString());
			return;
		}

		error.text = "";
		if(!Application.isEditor) Screen.orientation = test_orientations[test_orientation_index];
		Application.LoadLevel(currentScene);
	}

	public void ChangeOrientation()
	{
		test_orientation_index++;
		if(test_orientation_index >= test_orientations.Length) test_orientation_index = 0;
		orientationLabel.text = "Orientation: " + test_orientations[test_orientation_index].ToString();
	}
}
