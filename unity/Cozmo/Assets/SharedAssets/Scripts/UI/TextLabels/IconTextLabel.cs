using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using Anki.UI;

namespace Cozmo {
  namespace UI {
    public class IconTextLabel : MonoBehaviour {

      [SerializeField]
      private Image _Icon;

      [SerializeField]
      private AnkiTextLabel _TextLabel;

      public void SetIcon(Sprite sprite) {
        _Icon.sprite = sprite;
      }

      public void SetText(string text) {
        _TextLabel.text = text;
      }
    }
  }
}