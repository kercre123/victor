using UnityEngine;
using System.Collections.Generic;
using DG.Tweening;

public class UnlockAnimation : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _UnlockIconPrefab;
  private List<UnityEngine.UI.Image> _UnlockIconInstances = new List<UnityEngine.UI.Image>();

  [SerializeField]
  private float _TweenDuration = 0.15f;

  [SerializeField]
  private int _UnlockSpriteCount = 4;

  private Sequence _UnlockAnimationSequence;

  public void Initialize(Sprite unlockSprite, Transform container) {
    for (int i = 0; i < _UnlockSpriteCount; ++i) {
      UnityEngine.UI.Image unlockIcon = GameObject.Instantiate(_UnlockIconPrefab.gameObject).GetComponent<UnityEngine.UI.Image>();
      unlockIcon.transform.SetParent(container, false);
      unlockIcon.overrideSprite = unlockSprite;
      _UnlockIconInstances.Add(unlockIcon);
    }

    _UnlockAnimationSequence = DOTween.Sequence();

    // set transparency and tweens
    for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
      Color c = _UnlockIconInstances[i].color;
      c.a = 0.3f;
      _UnlockIconInstances[i].color = c;

      // Assigning is necessary otherwise the tween will cause a null exception
      int unlockGrowIndex = i;
      _UnlockAnimationSequence.Append(_UnlockIconInstances[unlockGrowIndex].transform.DOScale(Vector3.one * 1.5f, _TweenDuration)
                                      .SetEase(Ease.OutQuad));
    }

    for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
      // Assigning is necessary otherwise the tween will cause a null exception
      int unlockShrinkIndex = i;
      _UnlockAnimationSequence.Append(_UnlockIconInstances[unlockShrinkIndex].transform.DOScale(Vector3.one, _TweenDuration)
                                      .SetEase(Ease.InQuad));
    }

    _UnlockAnimationSequence.OnComplete(() => {
      GameObject.Destroy(gameObject);
    });
  }

  private void OnDestroy() {
    if (_UnlockAnimationSequence != null) {
      _UnlockAnimationSequence.Kill();
    }

    for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
      if (_UnlockIconInstances[i] != null) {
        GameObject.Destroy(_UnlockIconInstances[i].gameObject);
      }
    }
  }

}
