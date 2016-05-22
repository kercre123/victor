using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuickQuitMinigameButton : MinigameWidget {

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 200.0f;
      private const float kAnimDur = 0.25f;

      public delegate void QuickQuitButtonHandler();

      public event QuickQuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private Cozmo.UI.CozmoButton _QuickQuitButtonInstance;

      public string DASEventViewController {
        get { return _QuickQuitButtonInstance.DASEventViewController; } 
        set { _QuickQuitButtonInstance.DASEventViewController = value; }
      }

      private void Awake() {
        _QuickQuitButtonInstance.Initialize(HandleQuitButtonTap, "quit_game_during_setup_button", "TBD");
      }

      public override void DestroyWidgetImmediately() {
        _QuickQuitButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      private void HandleQuitButtonTap() {
        if (QuitGameConfirmed != null) {
          QuitGameConfirmed();
        }
      }
    }
  }
}
