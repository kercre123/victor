using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class HowToPlayButton : MinigameWidget {

      private const float kAnimXOffset = -200.0f;
      private const float kAnimYOffset = -200.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private Cozmo.UI.CozmoButton _HowToPlayButtonInstance;

      [SerializeField]
      private HowToPlayView _HowToPlayViewPrefab;
      private HowToPlayView _HowToPlayViewInstance;

      private string _HowToPlayLocKey = null;
      private GameObject _HowToPlayViewContentPrefab = null;

      public string DASEventViewController {
        get { return _HowToPlayButtonInstance.DASEventViewController; } 
        set { _HowToPlayButtonInstance.DASEventViewController = value; }
      }

      public void Initialize(string howToPlayTextLocKey, GameObject howToPlayViewContents) {
        _HowToPlayLocKey = howToPlayTextLocKey;
        _HowToPlayViewContentPrefab = howToPlayViewContents;
      }

      private void Start() {
        _HowToPlayButtonInstance.DASEventButtonName = "how_to_play_button";
        _HowToPlayButtonInstance.onClick.AddListener(HandleHowToPlayButtonTap);
      }

      public override void DestroyWidgetImmediately() {
        _HowToPlayButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(kAnimXOffset, kAnimYOffset, kAnimDur);
      }

      private void HandleHowToPlayButtonTap() {
        if (!IsHowToPlayViewOpen) {
          OpenHowToPlayView(null, null);
        }
      }

      public void OpenHowToPlayView(bool? overrideBackgroundDim, bool? overrideCloseOnTouchOutside) {
        _HowToPlayViewInstance = UIManager.OpenView(_HowToPlayViewPrefab, 
          overrideBackgroundDim: overrideBackgroundDim,
          overrideCloseOnTouchOutside: overrideCloseOnTouchOutside
        );
        if (_HowToPlayViewContentPrefab != null) {
          _HowToPlayViewInstance.Initialize(_HowToPlayViewContentPrefab);
        }
        else {
          _HowToPlayViewInstance.Initialize(_HowToPlayLocKey);
        }
      }

      public void CloseHowToPlayView() {
        if (IsHowToPlayViewOpen) {
          _HowToPlayViewInstance.CloseView();
          _HowToPlayViewInstance = null;
        }
      }

      public bool IsHowToPlayViewOpen {
        get {
          return _HowToPlayViewInstance != null;
        }
      }
    }
  }
}
