using UnityEngine;
using System.Collections;

public class ToggleBasedOnScreenSize : MonoBehaviour {

	[SerializeField] bool showOnLargeScreen = true;

	public void Awake() {
		float dpi = Screen.dpi;

		if(dpi == 0f) return;

		float screenHeightInches = (float)Screen.height / (float)dpi;
		gameObject.SetActive(showOnLargeScreen ^ screenHeightInches < CozmoUtil.SMALL_SCREEN_MAX_HEIGHT);
	}
}
