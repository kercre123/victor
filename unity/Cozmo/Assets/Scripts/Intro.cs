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

	protected void Awake()
	{
		play.onClick.AddListener( Play );
	}

	protected void Play()
	{
		string errorText = null;
		int idInteger;
		if (!int.TryParse (id.text, out idInteger)) {
			errorText = "You must enter a valid id.";
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
				Application.LoadLevel ("Main");
			}
		}

		error.text = errorText;
	}
}
