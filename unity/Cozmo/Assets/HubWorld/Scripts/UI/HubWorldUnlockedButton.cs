using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldUnlockedButton : HubWorldButton {

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

      StartCoroutine(DelayedBobAnimation());
    }

    private IEnumerator DelayedBobAnimation() {
      float delay = Random.Range(0f, 1f);
      yield return new WaitForSeconds(delay);

      // Start a bobbing animation that plays forever
      float duration = Random.Range(1.5f, 2f);
      float yOffset = Random.Range(10f, 15f);
      yOffset = Random.Range(0f, 1f) > 0.5f ? yOffset : -yOffset;
      Sequence bobbingSequence = DOTween.Sequence();
      bobbingSequence.SetLoops(-1, LoopType.Yoyo); 
      bobbingSequence.Append(transform.DOLocalMoveY(transform.localPosition.y - yOffset, duration).SetEase(Ease.InOutSine));
      bobbingSequence.Play();
    }
  }
}