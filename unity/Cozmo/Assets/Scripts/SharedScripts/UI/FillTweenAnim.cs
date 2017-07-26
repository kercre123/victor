using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.UI;
using DG.Tweening;

public class FillTweenAnim : MonoBehaviour {

  [SerializeField]
  private float _Duration_sec = 0f;

  [SerializeField]
  private float _Delay_sec = 0f;

  [SerializeField]
  private float _StartFillAmount = 0f;

  [SerializeField]
  private float _TargetFillAmount = 1f;

  private Image _Image = null;

  private void OnEnable() {
    _Image = GetComponent<Image>();

    if (_Image != null) {
      _Image.fillAmount = _StartFillAmount;
      _Image.DOFillAmount(_TargetFillAmount, _Duration_sec).SetEase(Ease.OutQuad).SetDelay(_Delay_sec);
    }
  }
}
