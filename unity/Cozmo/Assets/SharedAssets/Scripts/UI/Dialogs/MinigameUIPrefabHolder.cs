using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class MinigameUIPrefabHolder : ScriptableObject {

      private static MinigameUIPrefabHolder _sInstance;

      public static void SetInstance(MinigameUIPrefabHolder instance) {
        _sInstance = instance;
      }

      public static MinigameUIPrefabHolder Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private Cozmo.MinigameWidgets.SharedMinigameView _SharedMinigameViewPrefab;

      public Cozmo.MinigameWidgets.SharedMinigameView SharedMinigameViewPrefab {
        get { return _SharedMinigameViewPrefab; }
      }

      [SerializeField]
      private ChallengeEndedDialog _ChallengeEndViewPrefab;

      public ChallengeEndedDialog ChallengeEndViewPrefab {
        get { return _ChallengeEndViewPrefab; }
      }

      [SerializeField]
      private GameObject _ShowCozmoCubeSlide;

      public GameObject InitialCubesSlide {
        get { return _ShowCozmoCubeSlide; }
      }
    }
  }
}