using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour {
	[SerializeField] protected InputField id;
	[SerializeField] protected InputField ip;
	[SerializeField] protected InputField scene;
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

	private string lastScene
	{
		get { return PlayerPrefs.GetString("LastScene", "ControlSchemeTest"); }
		
		set { PlayerPrefs.SetString("LastScene", value); }
	}

	protected void Awake() {
		ip.text = lastIp;
		id.text = lastId;
		simulated.isOn = lastSimulated;
		scene.text = lastScene;
	}

	protected void Update() {
		if(connecting && Time.time > hackWait) {
			if(RobotEngineManager.instance.IsRobotConnected(CurrentRobotID)) {
				connecting = false;
				error.text = "";
				//RobotEngineManager.instance.DriveWheels(CurrentRobotID, 50.0f, 50.0f);

				Application.LoadLevel(scene.text);
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
				lastIp = ip.text;
				CurrentRobotID = idInteger;
				hackWait = Time.time + 5.0f;
				errorText = "Connecting...";
			}
		}

		error.text = errorText;
	}

	public void FakeTest() {
		Application.LoadLevel(scene.text);
	}

}
