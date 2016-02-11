using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeTitleWidget : MinigameWidget {

      [SerializeField]
      private Cozmo.UI.IconProxy _ChallangeIcon;

      [SerializeField]
      private Anki.UI.AnkiTextLabel _ChallengeTitleLabel;

      public string Text {
        set {
          _ChallengeTitleLabel.text = value;
        }
      }

      public Sprite Icon {
        set {
          _ChallangeIcon.SetIcon(value);
        }
      }

      #region IMinigameWidget

      public override void DestroyWidgetImmediately() {
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public override Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public override Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      #endregion
    }
  }
}