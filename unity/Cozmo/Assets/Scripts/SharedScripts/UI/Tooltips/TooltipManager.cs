using UnityEngine;

namespace Cozmo.UI {
  public class TooltipManager : MonoBehaviour {
    [SerializeField]
    private TooltipWidget[] _TooltipPrefabs;

    [SerializeField]
    private float _TooltipShowTime_Sec = 5.0f;

    [SerializeField]
    private Vector2 _MinBorderSpaceToEdge;

    public Vector2 MinBorderSpaceToEdge {
      get {
        return _MinBorderSpaceToEdge;
      }
    }

    private static TooltipManager _Instance;

    private TooltipWidget _CurrToolTip;

    private void Start() {
      _Instance = this;

      // Destroy listener on disconnect
      BaseView.BaseViewClosed += HandleBaseViewClosed;
    }

    private void OnDestroy() {
      CancelInvoke();

      BaseView.BaseViewClosed -= HandleBaseViewClosed;
    }

    public static TooltipManager Instance {
      get {
        if (_Instance == null) {
          string stackTrace = System.Environment.StackTrace;
          DAS.Error("TooltipManager.NullInstance", "Do not access TooltipManager until start");
          DAS.Debug("TooltipManager.NullInstance.StackTrace", DASUtil.FormatStackTrace(stackTrace));
          HockeyApp.ReportStackTrace("TooltipManager.NullInstance", stackTrace);
        }
        return _Instance;
      }
      private set {
        if (_Instance != null) {
          DAS.Error("TooltipManager.DuplicateInstance", "TooltipManager Instance already exists");
        }
        _Instance = value;
      }
    }

    private void HandleBaseViewClosed(BaseView view) {
      HideToolTip();
    }

    private void Update() {
      if (_CurrToolTip != null && _CurrToolTip.IsInited) {
        if (Input.GetMouseButtonDown(0)) {
          HideToolTip();
        }
      }
    }

    public void ShowToolTip(string header, string body, Transform parentTransform, Vector2 localPos, TooltipWidget.CaretPosition preferredDir) {
      HideToolTip();
      GameObject go = UIManager.CreateUIElement(_TooltipPrefabs[(int)preferredDir], parentTransform);
      _CurrToolTip = go.GetComponent<TooltipWidget>();
      _CurrToolTip.Init(header, body, localPos, preferredDir);
      Invoke("HideToolTip", _TooltipShowTime_Sec);
    }

    private void HideToolTip() {
      if (_CurrToolTip != null) {
        Destroy(_CurrToolTip.gameObject);
        _CurrToolTip = null;
        CancelInvoke("HideToolTip");
      }
    }

    public void SetToolTipEnabled(GameObject go, bool isEnabled) {
      TooltipGenericBaseDataComponent tooltipData = go.GetComponent<TooltipGenericBaseDataComponent>();
      if (tooltipData != null) {
        tooltipData.SetTooltipEnabled(isEnabled);
      }
    }

    public void SetToolTipMessage(GameObject go, int messageID) {
      TooltipGenericBaseDataComponent tooltipData = go.GetComponent<TooltipGenericBaseDataComponent>();
      if (tooltipData != null) {
        tooltipData.SetTooltipEnabled(true);
        tooltipData.SetShowBodyIndex(messageID);
      }
    }

  }

}