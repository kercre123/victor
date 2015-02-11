using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
	[SerializeField] protected InputField id;
	[SerializeField] protected InputField ip;
	[SerializeField] protected InputField visualizerIP;
	[SerializeField] protected Toggle simulated;
	[SerializeField] protected Button play;
	[SerializeField] protected Text error;

	private bool connecting = false;
	private float hackWait = 0.0f;

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

	private string lastSceneName
	{
		get { 
			string sceneName = PlayerPrefs.GetString("LastSceneName", "ThumbStick");
			return sceneName;
		}
		
		set { PlayerPrefs.SetString("LastSceneName", value); }
	}

	protected void OnEnable() {
		ip.text = lastIp;
		id.text = lastId;
		simulated.isOn = lastSimulated;

		visualizerIP.text = lastVisualizerIp;

	}

	protected void Update() {
		if(connecting && Time.time > hackWait) {
			if(RobotEngineManager.instance.IsRobotConnected(CurrentRobotID)) {
				connecting = false;
				error.text = "";
				Application.LoadLevel(lastSceneName);
			}
		}
	}

	public void PlayScheme(string choice) {
		lastSceneName = choice;
		Play();
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
					hackWait = Time.time + 2f;
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
	}

	public void FakeTest() {
		SaveData();
		Application.LoadLevel(lastSceneName);
	}

}
