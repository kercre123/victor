using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo.Upgrades {

  // Lives on CozmoUnlocksPanel prefab as a second component
  public class PyramidCubeUprightController : MonoBehaviour {
    [SerializeField]
    private PyramidCubeUprightModal _PyramidCubeUprightModalPrefab;
    private PyramidCubeUprightModal _PyramidCubeUprightModalInstance;

    [SerializeField]
    private float _ReopenModalCooldown_sec;

    private float _ReopenModalCooldownStartTimestamp;
    private bool _ShouldReopenModalAtEndOfCooldown;

    private void Start() {
      RobotEngineManager.Instance.AddCallback<BuildPyramidPreReqsChanged>(HandleBuildPyramidPreReqsChanged);
      RobotEngineManager.Instance.AddCallback<SparkEnded>(HandleSparkEnded);
      _ReopenModalCooldownStartTimestamp = -1;
      _ShouldReopenModalAtEndOfCooldown = false;
    }

    private void OnDestroy() {
      CloseCubeShouldBeUprightModal();
      RobotEngineManager.Instance.RemoveCallback<BuildPyramidPreReqsChanged>(HandleBuildPyramidPreReqsChanged);
      RobotEngineManager.Instance.RemoveCallback<SparkEnded>(HandleSparkEnded);
    }

    private void Update() {
      if (!_ReopenModalCooldownStartTimestamp.IsNear(-1, float.Epsilon)
          && (_ReopenModalCooldown_sec <= (Time.time - _ReopenModalCooldownStartTimestamp)
              && (_ShouldReopenModalAtEndOfCooldown))) {
        OpenCubeShouldBeUprightModal();
      }
    }

    private void HandleBuildPyramidPreReqsChanged(BuildPyramidPreReqsChanged message) {
      if (!message.areCubesUpright) {
        if (_ReopenModalCooldownStartTimestamp.IsNear(-1, float.Epsilon)) {
          OpenCubeShouldBeUprightModal();
        }
        else {
          _ShouldReopenModalAtEndOfCooldown = true;
        }
      }
      else {
        CloseCubeShouldBeUprightModal();
      }
    }

    private void HandleCubeUprightModalClosedByUser() {
      _ReopenModalCooldownStartTimestamp = Time.time;
    }

    private void OpenCubeShouldBeUprightModal() {
      UIManager.OpenModal(_PyramidCubeUprightModalPrefab,
                          new UI.ModalPriorityData(UI.ModalPriorityLayer.High, 0, UI.LowPriorityModalAction.CancelSelf, UI.HighPriorityModalAction.Stack),
                          (baseModal) => {
                            _PyramidCubeUprightModalInstance = (PyramidCubeUprightModal)baseModal;
                            _PyramidCubeUprightModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished += HandleCubeUprightModalClosedByUser;
                            _ShouldReopenModalAtEndOfCooldown = false;
                            _ReopenModalCooldownStartTimestamp = -1;
                          });
    }

    private void HandleSparkEnded(SparkEnded message) {
      CloseCubeShouldBeUprightModal();
    }

    private void CloseCubeShouldBeUprightModal() {
      if (_PyramidCubeUprightModalInstance != null) {
        _PyramidCubeUprightModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished -= HandleCubeUprightModalClosedByUser;
        UIManager.CloseModal(_PyramidCubeUprightModalInstance);
      }
      _ShouldReopenModalAtEndOfCooldown = false;
    }
  }
}
