using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace PatternPlay {

  public class PatternDiscoveredDialog : BaseView {

    [SerializeField]
    private PatternDiscoveryDisplay _PatternDiscoveryDisplayPrefab;
    private PatternDiscoveryDisplay _PatternDiscoveryDisplayInstance;

    [SerializeField]
    private Image[] _ImagesToFadeOnClose;

    [SerializeField]
    private Text[] _TextToFadeOnClose;

    public void Initialize(BlockPattern discoveredPattern) {
      GameObject display = UIManager.CreateUIElement(_PatternDiscoveryDisplayPrefab.gameObject);
      _PatternDiscoveryDisplayInstance = display.GetComponent<PatternDiscoveryDisplay>();
      _PatternDiscoveryDisplayInstance.Initialize(discoveredPattern);
    }

    protected override void CleanUp() {
      GameObject.Destroy(_PatternDiscoveryDisplayInstance.gameObject);
    }

    public void OnCloseButtonTap() {
      UIManager.CloseView(this);
    }

    protected override void ConstructCloseAnimation(Sequence closeAnimation) {
      int objectsFaded = 0;
      float endValue = 0f;
      float fadeDuration = 0.25f;
      foreach (Image image in _ImagesToFadeOnClose) {
        if (objectsFaded == 0) {
          closeAnimation.Append(image.DOFade(endValue, fadeDuration));
        }
        else {
          closeAnimation.Join(image.DOFade(endValue, fadeDuration));
        }
        objectsFaded++;
      }
      foreach (Text text in _TextToFadeOnClose) {
        if (objectsFaded == 0) {
          closeAnimation.Append(text.DOFade(endValue, fadeDuration));
        }
        else {
          closeAnimation.Join(text.DOFade(endValue, fadeDuration));
        }
        objectsFaded++;
      }

      _PatternDiscoveryDisplayInstance.AddCloseAnimationSequence(closeAnimation);
    }
  }

}
