using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

namespace Cozmo {
  namespace UI {
    [RequireComponent(typeof(Button))]
    public class TouchCatcher : MonoBehaviour {

      public event Action OnTouch;

      public void HandleButtonClick() {
        var callback = OnTouch;
        if (callback != null) {
          callback();
        }
      }

      public void Enable() {
        gameObject.SetActive(true);
        transform.SetAsLastSibling();
      }

      public void Disable() {
        gameObject.SetActive(false);
        OnTouch = null;
      }
    }
  }
}