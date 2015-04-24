using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class PrefSelector : GameObjectSelector {

	[SerializeField] string preferrence = null;

	bool optionsMenuWasOpenLastFrame = false;

	protected override void OnEnable() {
		RefreshFromPref();
	}

	void Update() {

		bool optionsOpenThisFrame = OptionsScreen.Open;

		if(optionsMenuWasOpenLastFrame && !optionsOpenThisFrame) {
			RefreshFromPref();


		}

		optionsMenuWasOpenLastFrame = optionsOpenThisFrame;
	}

	void RefreshFromPref() {
		if(!string.IsNullOrEmpty(preferrence)) {
			index = PlayerPrefs.GetInt(preferrence, 0);
		}
	}
}
