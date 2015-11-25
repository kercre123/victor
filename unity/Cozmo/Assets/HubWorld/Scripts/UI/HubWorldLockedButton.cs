using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using DG.Tweening;

namespace Cozmo.HubWorld {
  public class HubWorldLockedButton : HubWorldButton {

    private Sequence _BobbingSequence;

    [SerializeField]
    private Image _LockedChallengeImage;

    [SerializeField]
    private Sprite[] _LockedChallengeIcons;

    float _RotateSpeed = 10;

    public override void Initialize(ChallengeData challengeData) {

      base.Initialize(challengeData);

      if (_LockedChallengeImage != null) {
        int index = Random.Range(0, _LockedChallengeIcons.Length - 1);
        _LockedChallengeImage.sprite = _LockedChallengeIcons[index];
      }

      // StartCoroutine(DelayedBobAnimation());
      _RotateSpeed = Random.Range(-1f, 1f);
    }

    protected override void OnUpdate() {
      transform.RotateAround(transform.parent.position, Vector3.forward, _RotateSpeed * Time.deltaTime);
      transform.localRotation = Quaternion.identity;
    }

    private void OnDestroy() {
      if (_BobbingSequence != null) {
        _BobbingSequence.Kill();
      }
    }

    private IEnumerator DelayedBobAnimation() {
      float delay = Random.Range(0f, 1f);
      yield return new WaitForSeconds(delay);

      // Start a bobbing animation that plays forever
      float duration = Random.Range(3.5f, 5.5f);
      float yOffset = Random.Range(8f, 15f);
      yOffset = Random.Range(0f, 1f) > 0.5f ? yOffset : -yOffset;
      float xOffset = Random.Range(8f, 15f);
      xOffset = Random.Range(0f, 1f) > 0.5f ? xOffset : -xOffset;
      float targetScale = Random.Range(0.9f, 1.1f);
      _BobbingSequence = DOTween.Sequence();
      _BobbingSequence.SetLoops(-1, LoopType.Yoyo);  
      _BobbingSequence.Append(transform.DOLocalMove(new Vector3(transform.localPosition.x - xOffset,
        transform.localPosition.y - yOffset,
        transform.localPosition.z), duration).SetEase(Ease.InOutSine));
      _BobbingSequence.Join(transform.DOScale(new Vector3(targetScale, targetScale, 1), duration).SetEase(Ease.InOutSine));
      _BobbingSequence.Play();
    }
  }
}