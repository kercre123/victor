using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ModeTitleText : MonoBehaviour {
	Text text;

	void Awake() {
		text = GetComponent<Text>();
	}

	void OnEnable() {
		string game = PlayerPrefs.GetString("CurrentGame", "Slalom");
		text.text = game + " #" + PlayerPrefs.GetInt(game + "_CurrentLevel", 1);
	}
}
