using UnityEngine;
using System.Collections;

public class PatternPlayAudio : MonoBehaviour {

  [SerializeField]
  AudioClip[] _LightSounds;

  public void PlayLightsSound(int index) {
    AudioManager.PlayAudioClip(_LightSounds[index]);
  }
}
