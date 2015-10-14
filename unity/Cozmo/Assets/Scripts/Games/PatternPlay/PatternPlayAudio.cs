using UnityEngine;
using System.Collections;

public class PatternPlayAudio : MonoBehaviour {

  [SerializeField]
  AudioClip[] lightSounds;

  public void PlayLightsSound(int index) {
    AudioManager.PlayAudioClip(lightSounds[index], 0, AudioManager.Source.Gameplay, 1, 2);
  }
}
