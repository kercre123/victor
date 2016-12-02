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
      private AlertModal _BadLightAlertModalPrefab;

      public AlertModal BadLightAlertModalPrefab {
        get { return _BadLightAlertModalPrefab; }
      }

      [SerializeField]
      private AlertModal _LowBatteryAlertModalPrefab;

      public AlertModal LowBatteryAlertModalPrefab {
        get { return _LowBatteryAlertModalPrefab; }
      }

      [SerializeField]
      private AlertModal _ComingSoonAlertModalPrefab;

      public AlertModal ComingSoonAlertModalPrefab {
        get { return _ComingSoonAlertModalPrefab; }
      }

      [SerializeField]
      private ScrollingTextView _ScrollingTextViewPrefab;

      public ScrollingTextView ScrollingTextViewPrefab {
        get { return _ScrollingTextViewPrefab; }
      }

      [SerializeField]
      private LongConfirmationView _LongConfirmationViewPrefab;

      public LongConfirmationView LongConfirmationViewPrefab {
        get { return _LongConfirmationViewPrefab; }
      }

      [SerializeField]
      private BaseModal _CubeHelpViewPrefab;
      public BaseModal CubeHelpViewPrefab {
        get { return _CubeHelpViewPrefab; }
      }
    }
  }
}