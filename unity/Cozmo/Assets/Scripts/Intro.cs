using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class Intro : MonoBehaviour
{
	[SerializeField]
	protected InputField id;
	[SerializeField]
	protected InputField ip;
	[SerializeField]
	protected Toggle simulated;
	[SerializeField]
	protected Button play;
	[SerializeField]
	protected Text error;

	private bool connecting = false;

	public int CurrentRobotID { get; private set; }

	protected void Awake()
	{
		play.onClick.AddListener( Play );
	}

	protected void Update()
	{
		if (connecting) {
			if (RobotEngineManager.instance.IsRobotConnected(CurrentRobotID)) {
				connecting = false;
				error.text = "";
				Application.LoadLevel ("Main");
			}
		}
	}

	protected void Play()
	{
		CurrentRobotID = 0;

		string errorText = null;
		int idInteger;
		if (!int.TryParse (id.text, out idInteger) || idInteger == 0) {
			errorText = "You must enter a nonzero id.";
		}
		if (string.IsNullOrEmpty (errorText) && string.IsNullOrEmpty (ip.text)) {
			errorText = "You must enter an ip address.";
		}
		if (string.IsNullOrEmpty(errorText)) {
			CozmoResult result = RobotEngineManager.instance.ForceAddRobot(idInteger, ip.text, simulated.isOn);
			if (result != CozmoResult.OK) {
				errorText = "Error attempting to add robot: " + result.ToString();
			}
			else {
				connecting = true;
				CurrentRobotID = idInteger;
				errorText = "Connecting...";
			}
		}

		error.text = errorText;
	}
}
