using UnityEngine;
using System.Collections;
using DG.Tweening;

public class PatternDiscoveryDisplay : MonoBehaviour {

  [SerializeField]
  private PatternDisplay _stackPatternDisplay;
  
  [SerializeField]
  private PatternDisplay _horizontalPatternDisplay;

  [SerializeField]
  private GameObject _animationAnchorPrefab;
  private GameObject _animationAnchorInstance;

  private void OnDestroy() {
    GameObject.Destroy (_animationAnchorInstance);
  }

  public void Initialize(BlockPattern discoveredPattern) {
    if (discoveredPattern.verticalStack) {
      _stackPatternDisplay.pattern = discoveredPattern;
      _horizontalPatternDisplay.pattern = null;
    } else {
      _stackPatternDisplay.pattern = null;
      _horizontalPatternDisplay.pattern = discoveredPattern;
    } 
    _animationAnchorInstance = UIManager.CreatePerspectiveUI (_animationAnchorPrefab);
  }

  public void AddCloseAnimationSequence(Sequence sequence)
  {
    sequence.Append(transform.DOLocalMove (_animationAnchorInstance.transform.localPosition, 0.5f));
    sequence.Join(transform.DOScale(0.05f, 0.5f));
  }
}
