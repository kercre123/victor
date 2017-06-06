using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.UI {
  public class BadLightAlertConroller : MonoBehaviour {

    [SerializeField]
    private AlertModal _BadLightAlertPrefab;
    private AlertModal _BadLightAlertInstance = null;

    public bool EnableBadLightAlerts { get; set; }

    private void Start() {
      RobotEngineManager.Instance.AddCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
    }

    private void OnDestroy() {
      if (_BadLightAlertInstance != null) {
        _BadLightAlertInstance.CloseDialog();
      }
      RobotEngineManager.Instance.RemoveCallback<Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage>(HandleEngineErrorCode);
    }

    private void HandleEngineErrorCode(Anki.Cozmo.ExternalInterface.EngineErrorCodeMessage message) {
      if (_BadLightAlertInstance != null && message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityGood) {
        _BadLightAlertInstance.CloseDialog();
      }
      else if (message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooBright ||
        message.errorCode == Anki.Cozmo.EngineErrorCode.ImageQualityTooDark) {
        CreateBadLightAlert();
      }
    }

    private void CreateBadLightAlert() {
      if (!EnableBadLightAlerts) {
        return;
      }

      System.Action<BaseModal> badLightAlertCreated = (alertModal) => {
        ContextManager.Instance.AppFlash(playChime: true);

        var badLightAlertData = new AlertModalData("bad_light_alert", null, showCloseButton: true);
        var badLightAlert = (AlertModal)alertModal;
        badLightAlert.InitializeAlertData(badLightAlertData);

        _BadLightAlertInstance = badLightAlert;
      };

      var badLightPriorityData = new ModalPriorityData(ModalPriorityLayer.VeryLow, 1,
                                                       LowPriorityModalAction.Queue,
                                                       HighPriorityModalAction.Stack);

      UIManager.OpenModal(_BadLightAlertPrefab, badLightPriorityData, badLightAlertCreated,
                overrideCloseOnTouchOutside: true);
    }
  }
}