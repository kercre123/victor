using UnityEngine;
using System.Collections;

public class OptionsButton : MonoBehaviour {
	public void Toggle(bool toggle) {
		if(OptionsScreen.instance == null) return;
		OptionsScreen.instance.Toggle(toggle);
	}
}
