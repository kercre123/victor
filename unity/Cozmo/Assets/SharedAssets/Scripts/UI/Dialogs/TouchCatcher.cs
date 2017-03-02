using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

namespace Cozmo {
  namespace UI {
    [RequireComponent(typeof(Cozmo.UI.CozmoButtonLegacy))]
    public class TouchCatcher : MonoBehaviour {

      public event Action OnTouch;

      [SerializeField]
      private Cozmo.UI.CozmoButtonLegacy _FullScreenButton;

      public string DASEventButtonName {
        get { return _FullScreenButton.DASEventButtonName; }
        set { _FullScreenButton.DASEventButtonName = value; }
      }

      public string DASEventViewController {
        get { return _FullScreenButton.DASEventViewController; } 
        set { _FullScreenButton.DASEventViewController = value; }
      }

      public void Initialize(string dasEventButtonName, string dasEventViewController) {
        _FullScreenButton.Initialize(HandleButtonClick, dasEventButtonName, dasEventViewController);
      }

      private void OnDestroy() {
        _FullScreenButton.onClick.RemoveListener(HandleButtonClick);
      }

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