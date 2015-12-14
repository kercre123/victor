using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldCompleteButton : HubWorldButton {

    private Sequence _BobbingSequence;

    public override void Initialize(ChallengeData challengeData) {

      base.Initialize(challengeData);
    }

    private void OnEnable() {
      DelayedBobAnimation();
    }

    private void OnDisable() {
      if (_BobbingSequence != null) {
        _BobbingSequence.Kill();
        _BobbingSequence = null;
      }
    }

    private void DelayedBobAnimation() {
      float delay = Random.Range(0f, 1f);        
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
      _BobbingSequence.SetDelay(delay);
      _BobbingSequence.Play();
    }
  }
}