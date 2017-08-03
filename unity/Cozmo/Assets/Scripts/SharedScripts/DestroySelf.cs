using System.Collections;
using UnityEngine;

//this was created to allow one-shot effects to be spawned without worrying about having to clean them up later
//used for the RewardEffect.prefab and StarAwardedEffect.prefab
public class DestroySelf : MonoBehaviour {

  [SerializeField]
  private float _Lifespan_sec = 10f;

  private Coroutine coroutine = null;

  private void OnEnable() {
    coroutine = StartCoroutine(DestroySelfAfterDelay());
  }

  private void OnDisable() {
    if (coroutine != null) {
      StopCoroutine(coroutine);
      coroutine = null;
    }
  }

  private IEnumerator DestroySelfAfterDelay() {
    yield return new WaitForSeconds(_Lifespan_sec);
    coroutine = null;
    Destroy(gameObject);
  }
}
