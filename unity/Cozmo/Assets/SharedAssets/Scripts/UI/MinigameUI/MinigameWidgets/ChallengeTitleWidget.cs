using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

namespace Cozmo {
  namespace MinigameWidgets {
    public class ChallengeTitleWidget : MonoBehaviour, IMinigameWidget {

      public delegate void HowToPlayButtonHandler();

      public event HowToPlayButtonHandler HowToPlayViewOpened;
      public event HowToPlayButtonHandler HowToPlayViewClosed;

      [SerializeField]
      private Anki.UI.AnkiButton _HowToPlayButton;

      [SerializeField]
      private HowToPlayView _HowToPlayViewPrefab;
      private HowToPlayView _HowToPlayViewInstance;
      [SerializeField]
      private Cozmo.UI.IconTextLabel _ChallengeTitleLabel;

      private GameObject _HowToPlayViewContentPrefab;

      public void Initialize(string titleText, Sprite titleIcon, GameObject howToPlayContentPrefab) {
        _HowToPlayViewContentPrefab = howToPlayContentPrefab;
        _ChallengeTitleLabel.SetText(titleText);
        _ChallengeTitleLabel.SetIcon(titleIcon);
      }

      private void Start() {
        _HowToPlayButton.onClick.AddListener(HandleHowToPlayButtonTap);
      }

      public void OpenHowToPlayDialog() {
        _HowToPlayViewInstance = UIManager.OpenView(_HowToPlayViewPrefab) as HowToPlayView;
        _HowToPlayViewInstance.Initialize(_HowToPlayViewContentPrefab);
        _HowToPlayViewInstance.ViewCloseAnimationFinished += HandleHowToPlayViewClosed;

        if (HowToPlayViewOpened != null) {
          HowToPlayViewOpened();
        }
      }

      public void CloseHowToPlayView() {
        if (_HowToPlayViewInstance != null) {
          _HowToPlayViewInstance.CloseView();
          _HowToPlayViewInstance = null;
        }
      }

      private void HandleHowToPlayButtonTap() {
        OpenHowToPlayDialog();
      }

      private void HandleHowToPlayViewClosed() {
        if (HowToPlayViewClosed != null) {
          HowToPlayViewClosed();
        }
      }

      #region IMinigameWidget

      public void DestroyWidgetImmediately() {
        _HowToPlayButton.onClick.RemoveAllListeners();
        Destroy(gameObject);
      }

      // TODO: Don't hardcode this
      public Sequence OpenAnimationSequence() {
        Sequence open = DOTween.Sequence();
        open.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).From().SetEase(Ease.OutQuad));
        return open;
      }

      // TODO: Don't hardcode this
      public Sequence CloseAnimationSequence() {
        Sequence close = DOTween.Sequence();
        close.Append(this.transform.DOLocalMove(new Vector3(this.transform.localPosition.x + 600, 
          this.transform.localPosition.y + 300, this.transform.localPosition.z),
          0.25f).SetEase(Ease.OutQuad));
        return close;
      }

      public void EnableInteractivity() {
        _HowToPlayButton.Interactable = true;
      }

      public void DisableInteractivity() {
        _HowToPlayButton.Interactable = false;
      }

      #endregion
    }
  }
}