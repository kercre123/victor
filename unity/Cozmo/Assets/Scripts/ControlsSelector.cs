using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class ControlsSelector : GameObjectSelector {

	void OnEnable() {
		index = PlayerPrefs.GetInt("ControlSchemeIndex", 0);
	}

}
