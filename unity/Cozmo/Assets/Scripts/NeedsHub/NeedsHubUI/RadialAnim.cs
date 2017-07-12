using UnityEngine.Serialization;
using UnityEngine;
using UnityEngine.UI;

public class RadialAnim : MonoBehaviour {

  [SerializeField]
  [FormerlySerializedAs("animationTime")]
  float _AnimationTime = 1f;

  [SerializeField]
  [FormerlySerializedAs("delayTime")]
  float _DelayTime = 1f;

  [SerializeField]
  [FormerlySerializedAs("fadeTime")]
  float _FadeTime = 1f;

  [SerializeField]
  [FormerlySerializedAs("onColor")]
  Color _OnColor;

  [SerializeField]
  [FormerlySerializedAs("offColor")]
  Color _OffColor;

  float currentLerpTime;
  bool isAnimating;
  float fadeLerpTime;
  float fadePercComplete;
  bool isFading;
  Image glowImage;

  private void OnEnable() {
    Invoke("RestartAnim", _DelayTime);
  }

  private void RestartAnim() {
    glowImage = gameObject.GetComponent<Image>();
    glowImage.color = _OnColor;
    glowImage.fillAmount = 0f;
    currentLerpTime = 0f;
    isAnimating = true;
  }

  private void Update() {
    if (isAnimating) {
      //increment timer once per frame
      currentLerpTime += Time.deltaTime;
      if (currentLerpTime > _AnimationTime) {
        currentLerpTime = _AnimationTime;

        isAnimating = false;

        isFading = true;
        fadeLerpTime = 0;
      }
      float perc = currentLerpTime / _AnimationTime;
      //perc = Mathf.Sin(perc * Mathf.PI * 0.5f);
      glowImage.fillAmount = perc;//*.01f;
    }
    if (isFading) {
      fadeLerpTime += Time.deltaTime;
      float fadePerc = fadeLerpTime / _FadeTime;
      Color fadeColor = Color.Lerp(_OnColor, _OffColor, fadePerc);

      if (fadeLerpTime > _FadeTime) {
        fadeLerpTime = _FadeTime;
        isFading = false;
      }
      glowImage.color = fadeColor;
    }
  }
}
