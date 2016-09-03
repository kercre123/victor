using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class AlertViewLoader : ScriptableObject {

      private static AlertViewLoader _sInstance;

      public static void SetInstance(AlertViewLoader instance) {
        _sInstance = instance;
      }

      public static AlertViewLoader Instance {
        get { return _sInstance; }
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

      [SerializeField]
      private AlertView _AlertViewPrefab_BadLight;

      public AlertView AlertViewPrefab_BadLight {
        get { return _AlertViewPrefab_BadLight; }
      }

      [SerializeField]
      private ScrollingTextView _ScrollingTextViewPrefab;

      public ScrollingTextView ScrollingTextViewPrefab {
        get { return _ScrollingTextViewPrefab; }
      }
    }
  }
}