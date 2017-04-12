using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class BackButton : MinigameWidget {

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 200.0f;
      private const float kAnimDur = 0.25f;

      public delegate void BackButtonHandler();

      public BackButtonHandler HandleBackTapped;

      [SerializeField]
      private Cozmo.UI.CozmoButtonLegacy _BackButtonInstance;

      public string DASEventViewController {
        get { return _BackButtonInstance.DASEventViewController; }
        set { _BackButtonInstance.DASEventViewController = value; }
      }

      private void Awake() {
        _BackButtonInstance.Initialize(HandleBackTap, "back_button", _BackButtonInstance.DASEventViewController);
      }

      public override void DestroyWidgetImmediately() {
        _BackButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      private void HandleBackTap() {
        if (HandleBackTapped != null) {
          HandleBackTapped();
        }
      }
    }
  }
}
