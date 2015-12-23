using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldCompleteButton : HubWorldButton {

    private Sequence _BobbingSequence;

    [SerializeField]
    private Image _GradientImage;

    [SerializeField]
    private Image _ChallengeIconImage;

    public override void Initialize(ChallengeData challengeData) {

      base.Initialize(challengeData);

      if (_ChallengeIconImage != null) {
        if (challengeData.ChallengeIcon != null) {
          _ChallengeIconImage.sprite = challengeData.ChallengeIcon;
        }
        else {
          _ChallengeIconImage.gameObject.SetActive(false);
          if (_GradientImage != null) {
            _GradientImage.gameObject.SetActive(false);
          }
        }
      }
    }

    private void OnEnable() {
      StartCoroutine(DelayedBobAnimation());
    }

    private void OnDisable() {
      StopAllCoroutines();
      if (_BobbingSequence != null) {
        _BobbingSequence.Kill();
        _BobbingSequence = null;
      }
    }

    // use a coroutine because Sequences don't allow
    // delay, and we don't want to jump the prefab by
    // on start
    private IEnumerator DelayedBobAnimation() {
      float delay = Random.Range(0f, 1f);

      yield return new WaitForSeconds(delay);
              
      // Start a bobbing animation that plays forever
      float duration = Random.Range(3.5f, 5.5f);
      float yOffset = Random.Range(10f, 25f);
      yOffset = Random.Range(0f, 1f) > 0.5f ? yOffset : -yOffset;
      float xOffset = Random.Range(10f, 25f);
      xOffset = Random.Range(0f, 1f) > 0.5f ? xOffset : -xOffset;
      _BobbingSequence = DOTween.Sequence();
      _BobbingSequence.SetLoops(-1, LoopType.Yoyo);  

      _BobbingSequence.Append(transform.DOLocalMove(new Vector3(transform.localPosition.x - xOffset,
        transform.localPosition.y - yOffset,
        transform.localPosition.z), duration).SetEase(Ease.InOutSine));
      _BobbingSequence.Play();
    }
  }
}