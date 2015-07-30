using UnityEngine;
using System.Collections;

/// <summary>
/// lets a button component open the options menu on click
/// </summary>
public class OptionsButton : MonoBehaviour {
	public void Toggle(bool toggle) {
		if(OptionsScreen.instance == null) return;
		OptionsScreen.instance.Toggle(toggle);
	}
}
