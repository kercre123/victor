using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class QualityText : MonoBehaviour {

	Text text;
	void Awake() {
		text = GetComponent<Text>();
	}

	void Update() {
		text.text = QualitySettings.names[QualitySettings.GetQualityLevel()];
	}

}
