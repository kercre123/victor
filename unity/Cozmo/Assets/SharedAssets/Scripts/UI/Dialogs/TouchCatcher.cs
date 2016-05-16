using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

namespace Cozmo {
  namespace UI {
    [RequireComponent(typeof(Cozmo.UI.CozmoButton))]
    public class TouchCatcher : MonoBehaviour {

      public event Action OnTouch;

      [SerializeField]
      private Cozmo.UI.CozmoButton _FullScreenButton;

      public string DASEventButtonName {
        get { return _FullScreenButton.DASEventButtonName; }
        set { _FullScreenButton.DASEventButtonName = value; }
      }

      public string DASEventViewController {
        get { return _FullScreenButton.DASEventViewController; } 
        set { _FullScreenButton.DASEventViewController = value; }
      }

      private void Start() {
        _FullScreenButton.onClick.AddListener(HandleButtonClick);
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