using UnityEngine;
using System.Collections;

public class AudioManager : MonoBehaviour {
  private static AudioManager instance = null;

  [SerializeField]
  private AudioSource audioSource_;

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
      DAS.Warn("AudioManager", "AudioManager is null");
      return null;
    }
    instance.audioSource_.clip = clip;
    instance.audioSource_.Stop(); 
    instance.audioSource_.Play();
    return instance.audioSource_;
  }

  public static void Stop() {
    if (instance == null) {
      DAS.Warn("AudioManager", "AudioManager is null");
      return;
    }
    instance.audioSource_.Stop();
  }
}
