using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldUnlockedButton : HubWorldButton {

    private Sequence _BobbingSequence;

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
        }
      }
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
      float delay = Random.Range(0f, 1.5f);

      // Start a bobbing animation that plays forever
      float duration = Random.Range(1.5f, 2f);
      float yOffset = Random.Range(10f, 17f);
      _BobbingSequence = DOTween.Sequence();
      _BobbingSequence.SetLoops(-1, LoopType.Yoyo); 
      _BobbingSequence.Append(transform.DOLocalMoveY(transform.localPosition.y - yOffset, duration).SetEase(Ease.InOutSine));
      _BobbingSequence.SetDelay(delay);
      _BobbingSequence.Play();
    }
  }
}