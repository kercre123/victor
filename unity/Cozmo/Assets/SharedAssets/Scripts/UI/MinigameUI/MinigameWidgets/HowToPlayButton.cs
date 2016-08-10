using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class HowToPlayButton : MinigameWidget {

      public delegate void HowToPlayButtonClickedHandler(bool openingHowToPlayView);

      public event HowToPlayButtonClickedHandler OnHowToPlayButtonClicked;

      private const float kAnimXOffset = 0f;
      private const float kAnimYOffset = 300.0f;
      private const float kAnimDur = 0.25f;

      [SerializeField]
      private Cozmo.UI.CozmoToggleButton _HowToPlayButtonInstance;

      [SerializeField]
      private HowToPlayView _HowToPlayViewPrefab;
      private HowToPlayView _HowToPlayViewInstance;
      private bool _TapDetected = false;

      private bool _IsHowToPlayViewOpen = false;

      private string _HowToPlayLocKey = null;
      private GameObject _HowToPlayViewContentPrefab = null;

      public bool IsHowToPlayViewOpen {
        get { return _IsHowToPlayViewOpen; }
        set {
          if (_HowToPlayButtonInstance != null) {
            _HowToPlayButtonInstance.ShowPressedStateOnRelease = value;
          }
          _IsHowToPlayViewOpen = value;
        }
      }

      public string DASEventViewController {
        get { return _HowToPlayButtonInstance.DASEventViewController; }
        set { _HowToPlayButtonInstance.DASEventViewController = value; }
      }

      public void Initialize(string howToPlayTextLocKey, GameObject howToPlayViewContents, string dasEventViewControllerName, Color toggleColor) {
        _HowToPlayLocKey = howToPlayTextLocKey;
        _HowToPlayViewContentPrefab = howToPlayViewContents;
        _HowToPlayButtonInstance.Initialize(HandleHowToPlayButtonTap, "open_how_to_play_view_button", dasEventViewControllerName);
        _HowToPlayButtonInstance.PressedTintColor = toggleColor;
      }

      private void Update() {
        _TapDetected = false;
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
        // INGO It seems like this method is firing twice off of one tap, but I can't figure out why.
        // AnkiButton seems to be firing once like it should
        if (!_TapDetected) {
          _TapDetected = true;
          if (IsHowToPlayViewOpen) {
            CloseHowToPlayView();
            IsHowToPlayViewOpen = false;
            if (OnHowToPlayButtonClicked != null) {
              OnHowToPlayButtonClicked(false);
            }
          }
          else {
            OpenHowToPlayView(null, null);
            IsHowToPlayViewOpen = true;
            if (OnHowToPlayButtonClicked != null) {
              OnHowToPlayButtonClicked(true);
            }
          }
        }
      }

      public void OpenHowToPlayView(bool? overrideBackgroundDim, bool? overrideCloseOnTouchOutside) {
        _HowToPlayViewInstance = UIManager.OpenView(_HowToPlayViewPrefab,
          overrideBackgroundDim: overrideBackgroundDim,
          overrideCloseOnTouchOutside: overrideCloseOnTouchOutside
        );
        _HowToPlayViewInstance.Initialize(_HowToPlayLocKey, _HowToPlayViewContentPrefab);
        _HowToPlayViewInstance.ViewClosed += () => {
          IsHowToPlayViewOpen = false;
          if (OnHowToPlayButtonClicked != null) {
            OnHowToPlayButtonClicked(false);
          }
        };
      }

      public void CloseHowToPlayView() {
        if (IsHowToPlayViewOpen) {
          _HowToPlayViewInstance.CloseView();
          _HowToPlayViewInstance = null;
          IsHowToPlayViewOpen = false;
        }
      }
    }
  }
}
