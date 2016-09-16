using UnityEngine;
using System.Collections;

public class WifiSlide0 : MonoBehaviour {

  [SerializeField]
  Anki.UI.AnkiTextLabel _iOSInstructionText;

  [SerializeField]
  Anki.UI.AnkiTextLabel _AndroidInstructionText;

  [SerializeField]
  UnityEngine.UI.Image _iOSSettingsIcon;

  private void Awake() {
#if UNITY_ANDROID
    _iOSInstructionText.gameObject.SetActive(false);
    _AndroidInstructionText.gameObject.SetActive(true);
    _iOSSettingsIcon.gameObject.SetActive(false);
#endif
  }
}
