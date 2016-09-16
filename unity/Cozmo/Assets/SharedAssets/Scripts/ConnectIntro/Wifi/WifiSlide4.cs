using UnityEngine;
using System.Collections;

public class WifiSlide4 : MonoBehaviour {

  [SerializeField]
  UnityEngine.UI.Image _HomeScreenImage;

  [SerializeField]
  Sprite _AndroidSpecificSprite;

  private void Awake() {
#if UNITY_ANDROID
    _HomeScreenImage.overrideSprite = _AndroidSpecificSprite;
#endif
  }
}
