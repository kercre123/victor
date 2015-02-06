using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ControlSchemeTester : MonoBehaviour {

	[SerializeField] GameObject[] schemes = null;
	[SerializeField] int defaultIndex = 0;
	[SerializeField] Text label = null;
	[SerializeField] Text orientationLabel = null;
	int index = 0;
	ScreenOrientation orientation = ScreenOrientation.Portrait;

	void Awake() {
		orientation = Screen.orientation;

		if(schemes == null || schemes.Length == 0) {
			enabled = false;
			return;
		}
		Input.multiTouchEnabled = true;
		index = Mathf.Clamp(defaultIndex, 0, schemes.Length - 1);
	}

	void Update() {
		if(schemes == null || schemes.Length == 0) {
			enabled = false;
			return;
		}

		for(int i=0; i<schemes.Length; i++) {
			if(schemes[i] == null)
				continue;
			schemes[i].SetActive(index == i);
			if(index == i) {
				if(label != null && label.text != schemes[i].name) {
					label.text = schemes[i].name;
				}
			}
		}
	}
	
	public void NextScheme() {
		index++;
		if(schemes != null && index >= schemes.Length)
			index = 0;
	}

	public void NextOrientation() {
		orientation++;
		if(orientation >= ScreenOrientation.AutoRotation)
			orientation = ScreenOrientation.Portrait;
		Screen.orientation = orientation;
		orientationLabel.text = orientation.ToString();
	}

	void OnGUI() {
		GUILayout.BeginArea(new Rect(Screen.width-300f, 300f, 300f, 300f));
		GUILayout.Label("RobotID("+Intro.CurrentRobotID+")");
		GUILayout.EndArea();
	}

}
