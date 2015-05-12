using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class GameModeLabel : MonoBehaviour {
	Text text;

	void Awake () {
		text = GetComponent<Text>();
	}

	void OnEnable () {
		text.text = PlayerPrefs.GetString("CurrentGame", "Slalom");
	}

}
