using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class UIPrefabHolder : ScriptableObject {

      private static UIPrefabHolder _Instance;

      public static UIPrefabHolder Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<UIPrefabHolder>("Prefabs/UI/UIPrefabHolder");
          }
          return _Instance;
        }
      }

      [SerializeField]
      private GameObject _FullScreenButtonPrefab;

      public GameObject FullScreenButtonPrefab {
        get { return _FullScreenButtonPrefab; }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab;

      public AlertView AlertViewPrefab {
        get { return _AlertViewPrefab; }
      }

      [SerializeField]
      private Cozmo.MinigameWidgets.SharedMinigameView _SharedMinigameViewPrefab;

      public Cozmo.MinigameWidgets.SharedMinigameView SharedMinigameViewPrefab {
        get { return _SharedMinigameViewPrefab; }
      }

      [SerializeField]
      private AlertView _ChallengeEndViewPrefab;

      public AlertView ChallengeEndViewPrefab {
        get { return _ChallengeEndViewPrefab; }
      }

      [SerializeField]
      private Anki.UI.AnkiButton _DefaultButtonPrefab;

      public Anki.UI.AnkiButton DefaultButtonPrefab {
        get { return _DefaultButtonPrefab; }
      }
    }
  }
}