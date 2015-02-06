using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class SpringSlider : Slider {

	[SerializeField] float duration = 0f;
	float fadeFromValue = 0f;
	float timeSinceTouch = 0f;
	bool wasPressed = false;

	void Update() {
		if(!IsPressed()) {
			if(wasPressed) {
				fadeFromValue = value;
			}

			timeSinceTouch += Time.deltaTime;
			if(duration <= 0f) {
				value = 0f;
			}
			else {
				float scalar = timeSinceTouch / duration;
				value = Mathf.Lerp(fadeFromValue, (minValue + maxValue) * 0.5f, scalar);
			}
			wasPressed = false;
		}
		else {
			wasPressed = true;
		}
	}

}
