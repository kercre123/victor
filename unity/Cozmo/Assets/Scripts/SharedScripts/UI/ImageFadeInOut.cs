using UnityEngine;
using System.Collections;
using DG.Tweening;

public class ImageFadeInOut : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _Image;

  private Tween _Tween = null;

  private void Start() {
    if (_Image == null) {
      Destroy(this);
      return;
    }
    FadeOut();
  }

  private void OnDestroy() {
    if (_Tween != null) {
      _Tween.Kill();
    }
  }

  private void FadeIn() {
    _Tween = DOTween.ToAlpha(() => _Image.color, x => _Image.color = x, 1.0f, Random.Range(1.0f, 1.5f)).OnComplete(FadeOut);
  }

  private void FadeOut() {
    _Tween = DOTween.ToAlpha(() => _Image.color, x => _Image.color = x, 0.0f, Random.Range(1.0f, 1.5f)).OnComplete(FadeIn);
  }
}
