using UnityEngine;
using System.Collections;

public class PatternPlayAudio : MonoBehaviour {

  [SerializeField]
  AudioClip[] lightSounds;

  [SerializeField]
  AudioClip lightInputReady;

  public void PlayLightsSound(int index) {
    AudioManager.PlayAudioClip(lightSounds[index], 0, AudioManager.Source.Gameplay, 1, 2);
  }

  public void PlayInputReady() {
    AudioManager.PlayAudioClip(lightInputReady, 0, AudioManager.Source.Gameplay, 1, 2);
  }
}
