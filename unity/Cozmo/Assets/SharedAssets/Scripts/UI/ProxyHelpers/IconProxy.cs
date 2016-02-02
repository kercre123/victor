using UnityEngine;
using UnityEngine.UI;

namespace Cozmo {
  namespace UI {
    public class IconProxy : MonoBehaviour {

      [SerializeField]
      private Image _IconImage;

      public void SetIcon(Sprite sprite) {
        _IconImage.sprite = sprite;
      }
    }
  }
}