using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

public class PatternDiscoveredDialog : BaseDialog {

  [SerializeField]
  private PatternDiscoveryDisplay _patternDiscoveryDisplayPrefab;
  private PatternDiscoveryDisplay _patternDiscoveryDisplayInstance;

  [SerializeField]
  private Image[] _imagesToFadeOnClose;

  [SerializeField]
  private Text[] _textToFadeOnClose;

  public void Initialize(BlockPattern discoveredPattern) {
    GameObject display = UIManager.CreatePerspectiveUI (_patternDiscoveryDisplayPrefab.gameObject);
    _patternDiscoveryDisplayInstance = display.GetComponent<PatternDiscoveryDisplay> ();
    _patternDiscoveryDisplayInstance.Initialize (discoveredPattern);
  }

  protected override void CleanUp() {
    GameObject.Destroy (_patternDiscoveryDisplayInstance.gameObject);
  }

  public void OnCloseButtonTap() {
    UIManager.CloseDialog (this);
  }

  protected override void ConstructCloseAnimation(Sequence closeAnimation) {
    int objectsFaded = 0;
    float endValue = 0f;
    float fadeDuration = 0.25f;
    foreach (Image image in _imagesToFadeOnClose) {
      if (objectsFaded == 0) {
        closeAnimation.Append(image.DOFade(endValue, fadeDuration));
      } else {
        closeAnimation.Join(image.DOFade(endValue, fadeDuration));
      }
      objectsFaded++;
    }
    foreach (Text text in _textToFadeOnClose) {
      if (objectsFaded == 0) {
        closeAnimation.Append(text.DOFade(endValue, fadeDuration));
      } else {
        closeAnimation.Join(text.DOFade(endValue, fadeDuration));
      }
      objectsFaded++;
    }

    _patternDiscoveryDisplayInstance.AddCloseAnimationSequence (closeAnimation);
  }
}
