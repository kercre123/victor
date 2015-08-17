using UnityEngine;
using UnityEngine.UI;
using System.Collections;

/// <summary>
/// show the current quality setting on screen in game
/// </summary>
public class QualityText : MonoBehaviour {

  Text text;
  void Awake() {
    text = GetComponent<Text>();
  }

  void Update() {
    text.text = QualitySettings.names[QualitySettings.GetQualityLevel()];
  }

}
