using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace MinigameWidgets {

    public class AnimationSlide : MonoBehaviour {

      [SerializeField]
      private Anki.UI.AnkiTextLabel _InfoTextLabel;

      [SerializeField]
      private RectTransform _AnimationContainer;

      public void Initialize(GameObject animationPrefab, string infoLocKey) {
        if (animationPrefab != null) {
          UIManager.CreateUIElement(animationPrefab, _AnimationContainer);
        }
        _InfoTextLabel.text = Localization.Get(infoLocKey);
      }
    }
  }
}