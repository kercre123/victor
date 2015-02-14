using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ControlSchemeTester : MonoBehaviour {

	[SerializeField] GameObject[] screens = null;
	[SerializeField] int defaultIndex = 0;
	[SerializeField] Text label = null;
	[SerializeField] Text[] orientationLabels = null;
	[SerializeField] Toggle reverseLikeACarToggle = null;

	public bool ReverseLikeACar {
		get {
			if(reverseLikeACarToggle == null) return false;
			return reverseLikeACarToggle.isOn;
		}
	}

	int _index = -1;
	private int index {
		get {
			return _index;
		}

		set {
			if(value != _index) {
				if(_index >= 0 && _index < screens.Length) Debug.Log("ControlSchemeTester screen changed from " + screens[_index].name + " to " + screens[value].name );
				_index = value;
				Refresh();
			}
		}
	}

	ScreenOrientation orientation = ScreenOrientation.Portrait;

	void OnEnable() {
		orientation = Screen.orientation;

		if(screens == null || screens.Length == 0) {
			enabled = false;
			return;
		}
		Input.multiTouchEnabled = true;
		index = Mathf.Clamp(defaultIndex, 0, screens.Length - 1);

		if(reverseLikeACarToggle != null) {
			reverseLikeACarToggle.isOn = PlayerPrefs.GetInt("ReverseLikeACar", 0) == 1;
		}
	}

	void Refresh() {
		if(screens == null || screens.Length == 0) {
			enabled = false;
			return;
		}

		_index = Mathf.Clamp(_index, 0, screens.Length - 1);

		//first disable the old screen(s)
		for(int i=0; i<screens.Length; i++) {
			if(screens[i] == null) continue;
			if(index == i) continue;
			screens[i].SetActive(false);
		}

		//then enable the new one
		screens[index].SetActive(true);
		
		//then refresh our test title field
		if(label != null && label.text != screens[index].name) {
			label.text = screens[index].name;
		}

	}

	public void NextOrientation() {

		orientation++;

		if(orientation >= ScreenOrientation.AutoRotation) orientation = ScreenOrientation.Portrait;

		Screen.orientation = orientation;

		if(orientationLabels != null) foreach(Text text in orientationLabels) text.text = orientation.ToString();
	}

//	void OnGUI() {
//		GUILayout.BeginArea(new Rect(Screen.width-300f, 300f, 300f, 300f));
//		GUILayout.Label("RobotID("+Intro.CurrentRobotID+")");
//		GUILayout.EndArea();
//	}

	public void Exit() {
		Application.LoadLevel("Shell");
	}

	public void SetScreenIndex(int i) {
		index = Mathf.Clamp(i, 0, screens.Length - 1);
	}

	public void ToggleReverseLikeACar(bool on) {
		bool toggled = reverseLikeACarToggle.isOn;

		for(int i=0; i<screens.Length; i++) {
			if(screens[i] == null) continue;
			RobotRelativeControls controls = screens[i].GetComponent<RobotRelativeControls>();
			if(controls == null) continue;
			controls.SetReverseLikeACar(toggled);
		}

		PlayerPrefs.SetInt("ReverseLikeACar", on ? 1 : 0);
	}

}
