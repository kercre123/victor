using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo.Upgrades.GuardDog {

  // Lives on NeedsHub prefab as an additional component
  public class GuardDogController : MonoBehaviour {
    [SerializeField]
    private GuardDogModal _GuardDogModalPrefab;
    private GuardDogModal _GuardDogModalPrefabInstance;

    public bool EnableGuardDogModal { get; set; }

    private void Start() {
      RobotEngineManager.Instance.AddCallback<GuardDogStart>(HandleGuardDogStart);
    }

    private void OnDestroy() {
      CloseGuardDogModal();
      RobotEngineManager.Instance.RemoveCallback<GuardDogStart>(HandleGuardDogStart);
    }

    private void OnApplicationPause(bool pauseStatus) {
      if (pauseStatus) {
        CloseGuardDogModal();
      }
    }

    private void HandleGuardDogStart(GuardDogStart guardDogMessage) {
      if (!EnableGuardDogModal) {
        return;
      }

      DAS.Debug("GuardDogController.HandleGuardDogStart", "Opening modal");
      UIManager.OpenModal(_GuardDogModalPrefab,
                          new UI.ModalPriorityData(UI.ModalPriorityLayer.Low, 3,
                                                   UI.LowPriorityModalAction.CancelSelf,
                                                   UI.HighPriorityModalAction.Stack),
                (baseModal) => {
                  _GuardDogModalPrefabInstance = (GuardDogModal)baseModal;
                  _GuardDogModalPrefabInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished += CloseGuardDogModal;
                  RobotEngineManager.Instance.AddCallback<GuardDogEnd>(HandleGuardDogEnd);
                  RobotEngineManager.Instance.AddCallback<BehaviorTransition>(HandleGuardDogEnd);
                });
    }

    private void HandleGuardDogEnd(object messageObject) {
      UIManager.CloseModal(_GuardDogModalPrefabInstance);
    }

    private void CloseGuardDogModal() {
      if (_GuardDogModalPrefabInstance != null) {
        _GuardDogModalPrefabInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished -= CloseGuardDogModal;
        RobotEngineManager.Instance.RemoveCallback<GuardDogEnd>(HandleGuardDogEnd);
        RobotEngineManager.Instance.RemoveCallback<BehaviorTransition>(HandleGuardDogEnd);
        _GuardDogModalPrefabInstance = null;
      }
    }
  }
}
