using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class QuickQuitMinigameButton : MinigameWidget {

      public delegate void QuickQuitButtonHandler();

      public event QuickQuitButtonHandler QuitGameConfirmed;

      [SerializeField]
      private AnkiButton _QuickQuitButtonInstance;

      private void Start() {
        _QuickQuitButtonInstance.onClick.AddListener(HandleQuitButtonTap);
      }

      public override void DestroyWidgetImmediately() {
        _QuickQuitButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public override Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y + 200, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public override Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y + 200, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      private void HandleQuitButtonTap() {
        if (QuitGameConfirmed != null) {
          QuitGameConfirmed();
        }
      }
    }
  }
}
