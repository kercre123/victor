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

        [SerializeField]
        private float _Delay = 0f;

        private bool _WaitingToPlay = false;
        private float _TimeUntilPlay = 0f;

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

          _WaitingToPlay = true;
          _TimeUntilPlay = _Delay;
        }

        private void Update() {
          if (_WaitingToPlay) {
            _TimeUntilPlay -= Time.deltaTime;
            if (_TimeUntilPlay < 0f) {
              GameAudioClient.PostAudioEvent(_AudioEventParameter);
              _WaitingToPlay = false;
            }
          }
        }
      }
    }
  }
}
