using UnityEngine;
using System.Collections;

public class DebugObject : MonoBehaviour {

	void Awake () {
		RefreshSettings();
		OptionsScreen.RefreshSettings += RefreshSettings;
	}

	void OnDestroy () {
		OptionsScreen.RefreshSettings -= RefreshSettings;
	}

	void RefreshSettings () {
		gameObject.SetActive(PlayerPrefs.GetInt("ShowDebugInfo", 0) == 1);
	}

}
