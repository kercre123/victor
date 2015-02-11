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
			errorText = "You must enter an ip address.";
		}

		error.text = errorText;

		if(string.IsNullOrEmpty(errorText)) {
			SaveData();
			CurrentRobotID = idInteger;
			RobotEngineManager.instance.Connect (ip.text);
		}
	}

	protected void SaveData()
	{
		lastIp = ip.text;
		lastId = id.text;
		lastSimulated = simulated.isOn;
		lastScene = scene.text;
	}

	public void FakeTest() {
		SaveData();
		Application.LoadLevel(scene.text);
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
