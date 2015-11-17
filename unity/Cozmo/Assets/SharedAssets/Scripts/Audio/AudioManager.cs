using UnityEngine;
using System.Collections;

public class AudioManager : MonoBehaviour {
  private static AudioManager instance = null;
  private static readonly IDAS _DAS = DAS.GetInstance(typeof(AudioSource));


  [SerializeField]
  private AudioSource _AudioSource;

  void Awake() {
    if (instance != null) {
      GameObject.Destroy(gameObject);
      return;
    }

    instance = this;
  }

  private void OnDestroy() {
    if (instance == this)
      instance = null;
  }

  public static AudioSource PlayAudioClip(AudioClip clip) {
    if (instance == null) {
      _DAS.Warn("AudioManager is null");
      return null;
    }
    instance._AudioSource.clip = clip;
    instance._AudioSource.Stop(); 
    instance._AudioSource.Play();
    return instance._AudioSource;
  }

  public static void Stop() {
    if (instance == null) {
      _DAS.Warn("AudioManager is null");
      return;
    }
    instance._AudioSource.Stop();
  }
}
