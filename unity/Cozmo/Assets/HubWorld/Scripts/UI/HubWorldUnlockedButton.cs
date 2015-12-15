using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldUnlockedButton : HubWorldButton {

    private Sequence _BobbingSequence;

    [SerializeField]
    private Image _ChallengeIconImage;

    private int _CoroutineStartFrame;

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
      _CoroutineStartFrame++;
      StartCoroutine(DelayedBobAnimation());
    }

    private void OnDisable() {
      _CoroutineStartFrame++;
      if (_BobbingSequence != null) {
        _BobbingSequence.Kill();
        _BobbingSequence = null;
      }
    }

    private IEnumerator DelayedBobAnimation() {
      float delay = Random.Range(0f, 1.5f);
      int startFrame = _CoroutineStartFrame;
      yield return new WaitForSeconds(delay);

      if (startFrame != _CoroutineStartFrame) {
        yield break;
      }

      // Start a bobbing animation that plays forever
      float duration = Random.Range(1.5f, 2f);
      float yOffset = Random.Range(10f, 17f);
      _BobbingSequence = DOTween.Sequence();
      _BobbingSequence.SetLoops(-1, LoopType.Yoyo); 
      _BobbingSequence.Append(transform.DOLocalMoveY(transform.localPosition.y - yOffset, duration).SetEase(Ease.InOutSine));
      _BobbingSequence.Play();
    }
  }
}