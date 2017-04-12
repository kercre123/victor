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
      private AnkiTextLegacy _TextLabel;

      [SerializeField]
      private AnkiTextLegacy _DescLabel;

      public void SetIcon(Sprite sprite) {
        if (_Icon != null) {
          _Icon.sprite = sprite;
        }
      }

      public void SetText(string text) {
        if (_TextLabel != null) {
          _TextLabel.text = text;
        }
      }

      public void SetDesc(string text) {
        if (_DescLabel != null) {
          _DescLabel.text = text;
        }
      }
    }
  }
}