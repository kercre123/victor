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

  private Sequence _GrowSeqeuence;
  private Sequence _ShrinkSeqeuence;

  public void Initialize(Sprite unlockSprite, Transform container) {
    for (int i = 0; i < _UnlockSpriteCount; ++i) {
      UnityEngine.UI.Image unlockIcon = GameObject.Instantiate(_UnlockIconPrefab.gameObject).GetComponent<UnityEngine.UI.Image>();
      unlockIcon.transform.SetParent(container, false);
      unlockIcon.overrideSprite = unlockSprite;
      _UnlockIconInstances.Add(unlockIcon);
    }

    _GrowSeqeuence = DOTween.Sequence();

    // set transparency and tweens
    for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
      Color c = _UnlockIconInstances[i].color;
      c.a = 0.3f;
      _UnlockIconInstances[i].color = c;

      int unlockIndex = i;

      Tween unlockTween = DOTween.To(() => _UnlockIconInstances[unlockIndex].transform.localScale, x => _UnlockIconInstances[unlockIndex].transform.localScale = x, Vector3.one * 1.5f, _TweenDuration);
      _GrowSeqeuence.Append(unlockTween);
    }

    _GrowSeqeuence.Play();
    _GrowSeqeuence.OnComplete(() => {

      _ShrinkSeqeuence = DOTween.Sequence();
      for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
        int unlockIndex = i;
        Tween shrinkTween = DOTween.To(() => _UnlockIconInstances[unlockIndex].transform.localScale, x => _UnlockIconInstances[unlockIndex].transform.localScale = x, Vector3.one, _TweenDuration);
        _ShrinkSeqeuence.Append(shrinkTween);
      }

      _ShrinkSeqeuence.Play();
      _ShrinkSeqeuence.OnComplete(() => {
        GameObject.Destroy(gameObject);
      });

    });
  }

  private void OnDestroy() {

    if (_GrowSeqeuence != null) {
      _GrowSeqeuence.Kill();
    }

    if (_ShrinkSeqeuence != null) {
      _ShrinkSeqeuence.Kill();
    }

    for (int i = 0; i < _UnlockIconInstances.Count; ++i) {
      if (_UnlockIconInstances[i] != null) {
        GameObject.Destroy(_UnlockIconInstances[i].gameObject);
      }
    }
  }

}
