using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ScoreWidget : MonoBehaviour, IMinigameWidget {

      [HideInInspector, System.NonSerialized]
      public float AnimationXOffset = 600f;

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

      private void Start() {
        _ScoreContainer.gameObject.SetActive(false);
        _RoundContainer.gameObject.SetActive(false);
        _WinnerContainer.gameObject.SetActive(false);
      }

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
    }
  }
}