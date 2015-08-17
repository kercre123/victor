using UnityEngine;
using UnityEngine.UI;
using System.Collections;

public class ModeTitleText : MonoBehaviour {
  Text text;

  void Awake() {
    text = GetComponent<Text>();
  }

  void OnEnable() {
    string modeTitle = PlayerPrefs.GetString("CurrentGame", "Unknown").ToUpper();

    if(modeTitle != "FREE PLAY") {
      modeTitle += " #" + PlayerPrefs.GetInt(modeTitle + "_CurrentLevel", 1);
    }

    text.text = modeTitle;
  }
}
