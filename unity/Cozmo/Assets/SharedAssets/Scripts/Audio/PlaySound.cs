using UnityEngine;

namespace Anki {
  namespace Cozmo {
    namespace Audio {
      /// <summary>
      /// Attach this script to a GameObject to play a sound.
      /// </summary>
      public class PlaySound : MonoBehaviour {

        /// <summary>
        /// Specifies a time we would want to play a sound.
        /// Add more here (and their corresponding handlers) if other needs arise.
        /// </summary>
        public enum PlayEventType
        {
          Awake,
          Start,
          OnEnable,
          OnDisable
        }

        [SerializeField]
        private AudioEventParameter _AudioEventParameter = AudioEventParameter.InvalidEvent;

        [SerializeField]
        private PlayEventType _PlayEvent = PlayEventType.Awake;

        private void Awake() {
          if (PlayEventType.Awake == _PlayEvent) {
            Play();
          }
        }

        private void Start() {
          if (PlayEventType.Start == _PlayEvent) {
            Play();
          }
        }

        private void OnEnable() {
          if (PlayEventType.OnEnable == _PlayEvent) {
            Play();
          }
        }

        private void OnDisable() {
          if (PlayEventType.OnDisable == _PlayEvent) {
            Play();
          }
        }

        private void Play() {
          if (_AudioEventParameter.IsInvalid()) {
            DAS.Warn("PlaySound.Play", "_AudioEventParameter is invalid.");
            return;
          }

          GameAudioClient.PostAudioEvent(_AudioEventParameter);
        }
      }
    }
  }
}
