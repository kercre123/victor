using UnityEngine;
using System.Collections;
using DG.Tweening;

public class ImageFadeInOut : MonoBehaviour {

  [SerializeField]
  private UnityEngine.UI.Image _Image;

  private void Start() {
    FadeOut();
  }

  private void FadeIn() {
    DOTween.ToAlpha(() => _Image.color, x => _Image.color = x, 1.0f, Random.Range(1.0f, 1.5f)).OnComplete(FadeOut);
  }

  private void FadeOut() {
    DOTween.ToAlpha(() => _Image.color, x => _Image.color = x, 0.0f, Random.Range(1.0f, 1.5f)).OnComplete(FadeIn);
  }
}
