using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class MinigameUIPrefabHolder : ScriptableObject {

      private static MinigameUIPrefabHolder _Instance;

      public static MinigameUIPrefabHolder Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<MinigameUIPrefabHolder>("Prefabs/UI/UIPrefabHolder");
          }
          return _Instance;
        }
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