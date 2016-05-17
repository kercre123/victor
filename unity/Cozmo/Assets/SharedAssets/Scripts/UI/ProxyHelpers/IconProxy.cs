using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class IconProxy : MonoBehaviour {

      [SerializeField]
      private Image _IconImage;

      [SerializeField]
      private bool _ShowBackground = true;

      [SerializeField]
      private Image _BackgroundImage;

      private void Start() {
        if (_BackgroundImage != null) {
          _BackgroundImage.gameObject.SetActive(_ShowBackground);
        }
      }

      public void SetIcon(Sprite sprite) {
        _IconImage.sprite = sprite;
      }

      public void SetAlpha(float alpha) {
        Color oldColor = _IconImage.color;
        oldColor.a = alpha;
        _IconImage.color = oldColor;
      }
    }
  }
}