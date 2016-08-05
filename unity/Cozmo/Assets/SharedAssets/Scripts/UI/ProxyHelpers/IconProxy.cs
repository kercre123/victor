using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class IconProxy : MonoBehaviour {

      public Image IconImage;

      [SerializeField]
      private bool _ShowBackground = true;

      [SerializeField]
      protected Image _BackgroundImage;

      private void Start() {
        if (_BackgroundImage != null) {
          _BackgroundImage.gameObject.SetActive(_ShowBackground);
        }
      }

      public void SetIcon(Sprite sprite) {
        IconImage.sprite = sprite;
      }

      public void SetAlpha(float alpha) {
        Color oldColor = IconImage.color;
        oldColor.a = alpha;
        IconImage.color = oldColor;
      }
    }
  }
}