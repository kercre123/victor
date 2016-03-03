using UnityEngine;
using System.Collections;
using UnityEngine.UI;
using System;

namespace Cozmo {
  namespace UI {
    [RequireComponent(typeof(Anki.UI.AnkiButton))]
    public class TouchCatcher : MonoBehaviour {

      public event Action OnTouch;

      [SerializeField]
      private Anki.UI.AnkiButton _FullScreenButton;

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