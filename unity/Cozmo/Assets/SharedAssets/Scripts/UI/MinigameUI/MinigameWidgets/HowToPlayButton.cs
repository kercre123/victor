using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class HowToPlayButton : MinigameWidget {

      public delegate void HowToPlayButtonClickedHandler();

      public event HowToPlayButtonClickedHandler OnHowToPlayButtonClicked;

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 300.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private Cozmo.UI.CozmoButton _HowToPlayButtonInstance;

      private void Awake() {
        _HowToPlayButtonInstance.Initialize(() => {
          if (OnHowToPlayButtonClicked != null) {
            OnHowToPlayButtonClicked();
          }
        }, "how_to_play_button", DASEventViewController);
      }

      public string DASEventViewController {
        get { return _HowToPlayButtonInstance.DASEventViewController; }
        set { _HowToPlayButtonInstance.DASEventViewController = value; }
      }

      public override void DestroyWidgetImmediately() {
        _HowToPlayButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, -kAnimYOffset, kAnimDur);
      }
    }
  }
}
