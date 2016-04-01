using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    [CreateAssetMenu]
    public class AlertViewHolder : ScriptableObject {

      private static AlertViewHolder _Instance;

      public static AlertViewHolder Instance {
        get {
          if (_Instance == null) {
            _Instance = Resources.Load<AlertViewHolder>("Prefabs/UI/UIPrefabHolder");
          }
          return _Instance;
        }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab;

      public AlertView AlertViewPrefab {
        get { return _AlertViewPrefab; }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab_NoText;

      public AlertView AlertViewPrefab_NoText {
        get { return _AlertViewPrefab_NoText; }
      }

      [SerializeField]
      private AlertView _AlertViewPrefab_Icon;

      public AlertView AlertViewPrefab_Icon {
        get { return _AlertViewPrefab_Icon; }
      }
    }
  }
}