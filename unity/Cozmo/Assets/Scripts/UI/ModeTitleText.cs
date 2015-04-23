using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ModeTitleText : MonoBehaviour {
	[SerializeField] Text title;

	void OnEnable() {
		string game = PlayerPrefs.GetString("CurrentGame", "Slalom");
		title.text = game + " #" + PlayerPrefs.GetInt(game + "_CurrentLevel", 1);
	}
}
