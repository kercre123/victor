using UnityEngine.Serialization;
using UnityEngine;

public class ShimmerAnim : MonoBehaviour {

  [SerializeField]
  [FormerlySerializedAs("animationTime")]
  float _AnimationTime = 1f;

  [SerializeField]
  [FormerlySerializedAs("delayTime")]
  float _DelayTime = 1f;

  [SerializeField]
  [FormerlySerializedAs("startPos")]
  Vector3 _StartPos;

  [SerializeField]
  [FormerlySerializedAs("endPos")]
  Vector3 _EndPos;

  float currentLerpTime;
  bool isAnimating;
  RectTransform rt;

  private void OnEnable() {
    Invoke("RestartAnim", _DelayTime);
  }

  private void RestartAnim() {
    rt = gameObject.GetComponent<RectTransform>();
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
      }

      //lerp!
      float perc = currentLerpTime / _AnimationTime;
      perc = Mathf.Sin(perc * Mathf.PI * 0.5f);

      rt.localPosition = Vector3.Lerp(_StartPos, _EndPos, perc);
    }
  }
}
