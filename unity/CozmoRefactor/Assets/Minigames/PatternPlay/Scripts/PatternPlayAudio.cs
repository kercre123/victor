using UnityEngine;
using System.Collections;

public class PatternPlayAudio : MonoBehaviour {

  [SerializeField]
  AudioClip[] lightSounds_;

  public void PlayLightsSound(int index) {
    AudioManager.PlayAudioClip(lightSounds_[index], 0, AudioManager.Source.Gameplay, 1, 2);
  }
}
