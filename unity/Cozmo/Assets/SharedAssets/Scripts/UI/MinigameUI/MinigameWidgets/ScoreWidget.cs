using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ScoreWidget : MonoBehaviour, IMinigameWidget {

      [SerializeField]
      private Image _PortraitImage;

      [SerializeField]
      private LayoutElement _ScoreContainer;

      [SerializeField]
      private AnkiTextLabel _ScoreCountLabel;

      [SerializeField]
      private LayoutElement _RoundContainer;

      [SerializeField]
      private SegmentedBar _RoundCountBar;

      [SerializeField]
      private RectTransform _WinnerContainer;

      public float AnimationXOffset {
        set;
        private get;
      }

      public int MaxRounds {
        set {
          Debug.LogError("Max rounds " + value);
          _RoundContainer.gameObject.SetActive(true);
          _RoundCountBar.SetMaximumSegments(value);
          _RoundCountBar.SetCurrentNumSegments(0);
        }
      }

      public Sprite Portrait {
        set {
          _PortraitImage.sprite = value;
        }
      }

      public int Score {
        set {
          Debug.LogError("Score " + value);
          _ScoreContainer.gameObject.SetActive(true);
          _ScoreCountLabel.text = Localization.GetNumber(value);
        }
      }

      public int RoundsWon {
        set {
          Debug.LogError("Rounds won " + value);
          _RoundCountBar.SetCurrentNumSegments(value);
        }
      }

      public bool IsWinner {
        set {
          _WinnerContainer.gameObject.SetActive(value);
        }
      }

      private void Awake() {
        _ScoreContainer.gameObject.SetActive(false);
        _RoundContainer.gameObject.SetActive(false);
        _WinnerContainer.gameObject.SetActive(false);
      }

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + AnimationXOffset, 
          this.transform.localPosition.y, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + AnimationXOffset, 
          this.transform.localPosition.y, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      #endregion
    }
  }
}