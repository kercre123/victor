using UnityEngine;
using System.Collections;

namespace Cozmo {
  namespace UI {
    public class AlertModalLoader : ScriptableObject {

      private static AlertModalLoader _sInstance;

      public static void SetInstance(AlertModalLoader instance) {
        _sInstance = instance;
      }

      public static AlertModalLoader Instance {
        get { return _sInstance; }
      }

      [SerializeField]
      private AlertModal _AlertModalPrefab;

      public AlertModal AlertModalPrefab {
        get { return _AlertModalPrefab; }
      }

      [SerializeField]
      private AlertModal _NoTextAlertModalPrefab;

      public AlertModal NoTextAlertModalPrefab {
        get { return _NoTextAlertModalPrefab; }
      }

      [SerializeField]
      private AlertModal _IconAlertModalPrefab;

      public AlertModal IconAlertModalPrefab {
        get { return _IconAlertModalPrefab; }
      }

      [SerializeField]
      private ScrollingTextModal _ScrollingTextModalPrefab;

      public ScrollingTextModal ScrollingTextModalPrefab {
        get { return _ScrollingTextModalPrefab; }
      }

      [SerializeField]
      private LongPressConfirmationModal _LongPressConfirmationModalPrefab;

      public LongPressConfirmationModal LongPressConfirmationModalPrefab {
        get { return _LongPressConfirmationModalPrefab; }
      }

      [SerializeField]
      private BaseModal _CubeHelpModalPrefab;

      public BaseModal CubeHelpModalPrefab {
        get { return _CubeHelpModalPrefab; }
      }
    }
  }
}