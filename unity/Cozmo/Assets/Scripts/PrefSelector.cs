using UnityEngine;
using UnityEngine.UI;
using System.Collections;


public class PrefSelector : GameObjectSelector {

	[SerializeField] string preferrence = null;

	void OnEnable() {
		if(!string.IsNullOrEmpty(preferrence)) {
			index = PlayerPrefs.GetInt(preferrence, 0);
		}
	}

}
