using UnityEngine;
using System.Collections;
using DG.Tweening;
using Anki.UI;
using Cozmo.UI;

namespace Cozmo {
  namespace MinigameWidgets {
    public class HowToPlayButton : MonoBehaviour, IMinigameWidget {

      [SerializeField]
      private AnkiButton _HowToPlayButtonInstance;

      [SerializeField]
      private HowToPlayView _HowToPlayViewPrefab;
      private HowToPlayView _HowToPlayViewInstance;
      private GameObject _HowToPlayViewContentPrefab;

      public void Initialize(GameObject howToPlayViewContents) {
        _HowToPlayViewContentPrefab = howToPlayViewContents;
      }

      private void Start() {
        _HowToPlayButtonInstance.onClick.AddListener(HandleHowToPlayButtonTap);
      }

      public void DestroyWidgetImmediately() {
        _HowToPlayButtonInstance.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y - 200, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x - 200, 
          this.transform.localPosition.y - 200, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      private void HandleHowToPlayButtonTap() {
        if (!IsHowToPlayViewOpen) {
          OpenHowToPlayView(null, null);
        }
      }

      public void OpenHowToPlayView(bool? overrideBackgroundDim, bool? overrideCloseOnTouchOutside) {
        _HowToPlayViewInstance = UIManager.OpenView(_HowToPlayViewPrefab, 
          animateImmediately: true,
          overrideBackgroundDim: overrideBackgroundDim,
          overrideCloseOnTouchOutside: overrideCloseOnTouchOutside
        ) as HowToPlayView;
        _HowToPlayViewInstance.Initialize(_HowToPlayViewContentPrefab);
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
