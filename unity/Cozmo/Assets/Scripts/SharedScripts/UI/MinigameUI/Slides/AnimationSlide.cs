using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace MinigameWidgets {

    public class AnimationSlide : MonoBehaviour {

      [SerializeField]
      private Anki.UI.AnkiTextLegacy _HeaderTextLabel;

      [SerializeField]
      private Anki.UI.AnkiTextLegacy _InfoTextLabel;

      [SerializeField]
      private RectTransform _AnimationContainer;

      public void Initialize(GameObject animationPrefab, string infoLocKey, string headerLocKey = null) {
        if (animationPrefab != null) {
          UIManager.CreateUIElement(animationPrefab, _AnimationContainer);
        }
        _InfoTextLabel.text = Localization.Get(infoLocKey);

        _HeaderTextLabel.gameObject.SetActive(headerLocKey != null);
        if (headerLocKey != null) {
          _HeaderTextLabel.text = Localization.Get(headerLocKey);
        }
      }
    }
  }
}