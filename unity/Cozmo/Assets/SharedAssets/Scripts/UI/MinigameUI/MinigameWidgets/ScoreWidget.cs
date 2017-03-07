using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ScoreWidget : MinigameWidget {

      private const float kAnimYOffset = 0.0f;
      private const float kAnimDur = 0.25f;

      private float _AnimationXOffset;

      public float AnimationXOffset {
        set {
          _AnimationXOffset = value;
          bool isOnLeft = (_AnimationXOffset < 0);

          // Flip background x scale if on left
          if (isOnLeft) {
            _Background.gameObject.transform.localScale = new Vector3(-1, 1, 1);
          }
        }
        private get { return _AnimationXOffset; }
      }

      [SerializeField]
      private Image _PortraitImage;

      [SerializeField]
      private GameObject _ScoreContainer;

      [SerializeField]
      private AnkiTextLegacy _ScoreNameLabel;

      [SerializeField]
      private AnkiTextLegacy _ScoreCountLabel;

      [SerializeField]
      private GameObject _RoundContainer;

      [SerializeField]
      private SegmentedBar _RoundCountBar;

      [SerializeField]
      private RectTransform _WinnerContainer;

      [SerializeField]
      private Canvas _WinnerCanvas;

      [SerializeField]
      private Image _Background;

      private Color _UndimmedTintColor = Color.white;

      public int MaxRounds {
        set {
          _RoundContainer.SetActive(true);
          _RoundCountBar.SetMaximumSegments(value);
          _RoundCountBar.SetCurrentNumSegments(0);
        }
      }

      public Sprite Portrait {
        set {
          _PortraitImage.sprite = value;
        }
      }

      public Color UnDimmedPortaitTintColor {
        set {
          _UndimmedTintColor = value;
        }
        get {
          return _UndimmedTintColor;
        }
      }
      public Color DimPortaitTintColor { get; set; }

      public int Score {
        set {
          _ScoreContainer.SetActive(true);
          _ScoreCountLabel.text = Localization.GetNumber(value);
        }
      }

      public int RoundsWon {
        set {
          _RoundCountBar.SetCurrentNumSegments(value);
        }
      }

      public bool IsWinner {
        set {
          _WinnerContainer.gameObject.SetActive(value);
          _ScoreNameLabel.gameObject.SetActive(!value);
        }
      }

      public bool IsPlayer {
        set {
          string locKey = "";
          if (value) {
            locKey = LocalizationKeys.kMinigameTextScorePlayer;
          }
          else {
            locKey = LocalizationKeys.kMinigameTextScoreCozmo;
          }
          _ScoreNameLabel.text = Localization.Get(locKey); ;
        }
      }

      // For multiplayer, we want to set the name directly
      public void SetNameLabelText(string lblText) {
        _ScoreNameLabel.text = lblText;
      }

      public bool Dim {
        set {
          _PortraitImage.color = value ? DimPortaitTintColor : _UndimmedTintColor;
        }
      }

      private void Awake() {
        _ScoreContainer.SetActive(false);
        _RoundContainer.SetActive(false);
        _WinnerContainer.gameObject.SetActive(false);
        DimPortaitTintColor = Color.gray;
      }

      private void Update() {
        if (_WinnerContainer.gameObject.activeInHierarchy && _WinnerCanvas != null) {
          // Force the canvas to reposition itself
          _WinnerCanvas.gameObject.SetActive(false);
          _WinnerCanvas.gameObject.SetActive(true);
        }
      }

      #region IMinigameWidget

      public override void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      public override Sequence CreateOpenAnimSequence() {
        return CreateOpenAnimSequence(AnimationXOffset, kAnimYOffset, kAnimDur);
      }

      public override Sequence CreateCloseAnimSequence() {
        return CreateCloseAnimSequence(AnimationXOffset, kAnimYOffset, kAnimDur);
      }

      #endregion
    }
  }
}