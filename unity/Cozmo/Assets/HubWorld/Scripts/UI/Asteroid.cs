using System;
using DG.Tweening;
using UnityEngine;
using System.Collections;
using Random = UnityEngine.Random;

public class Asteroid : MonoBehaviour {

  private GameObject _Child;
  private Vector3 _SpinAxis;

  private void Awake() {

    var index = Random.Range(0, 25);
    var prefab = Resources.Load<GameObject>("Prefabs/Asteroids/Asteroid_" + index);

    _Child = (GameObject)GameObject.Instantiate(prefab);

    _Child.transform.SetParent(transform, false);

    _Child.transform.rotation = Quaternion.LookRotation(Random.onUnitSphere, Random.onUnitSphere);

    _SpinAxis = Random.onUnitSphere;
  }

  private Sequence _BobbingSequence;

  public void Initialize(ChallengeData challengeData) {

    StartCoroutine(DelayedBobAnimation());
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

  private void Update() {
    var angle = Mathf.LerpAngle(0f, 15f, Time.deltaTime);

    transform.Rotate(_SpinAxis, angle);

  }
}

