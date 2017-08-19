using UnityEngine;
using System.Collections;
using Anki.Cozmo.ExternalInterface;

namespace Cozmo.Upgrades {

  // Lives on SparksDetailModal prefab as a second component
  public class PyramidCubeUprightController : MonoBehaviour {
    [SerializeField]
    private PyramidCubeUprightModal _PyramidCubeUprightModalPrefab;
    private PyramidCubeUprightModal _PyramidCubeUprightModalInstance;

    [SerializeField]
    private float _ReopenModalCooldown_sec;

    private float _ReopenModalCooldownStartTimestamp;
    private const float kDontCheckModalTimestamp = -1;

    private bool _AreCubesUpright = true;

    private void Start() {
      RobotEngineManager.Instance.AddCallback<PyramidPreReqState>(HandleReceivedPyramidPreReqState);
      RobotEngineManager.Instance.AddCallback<HardSparkEndedByEngine>(HandleSparkComplete);
      _ReopenModalCooldownStartTimestamp = kDontCheckModalTimestamp;
    }

    private void OnDestroy() {
      CloseCubeShouldBeUprightModal();
      RobotEngineManager.Instance.RemoveCallback<PyramidPreReqState>(HandleReceivedPyramidPreReqState);
      RobotEngineManager.Instance.RemoveCallback<HardSparkEndedByEngine>(HandleSparkComplete);
    }

    private void OnApplicationPause(bool pauseStatus) {
      if (pauseStatus) {
        CloseCubeShouldBeUprightModal();
        _AreCubesUpright = true; // reset this flag since the spark ending won't necessarily set this.
      }
    }

    private void Update() {
      if (!_ReopenModalCooldownStartTimestamp.IsNear(kDontCheckModalTimestamp, float.Epsilon)){
        bool isTimeToReopenModal = 
               (_ReopenModalCooldown_sec <= (Time.time - _ReopenModalCooldownStartTimestamp)
               && (!_AreCubesUpright));
        if(isTimeToReopenModal){
          OpenCubeShouldBeUprightModal();
        }
      }
    }

    private void HandleReceivedPyramidPreReqState(PyramidPreReqState message) {
      if (_AreCubesUpright != message.areCubesUpright) {
        DAS.Debug ("PyramidCubeUprightController.HandleBuildPyramidPreReqsChanged", "cubes upright: " + message.areCubesUpright);
        if (!message.areCubesUpright) {
          _ReopenModalCooldownStartTimestamp = Time.time - _ReopenModalCooldown_sec;
        } else {
          CloseCubeShouldBeUprightModal();
          _ReopenModalCooldownStartTimestamp = kDontCheckModalTimestamp;
        }
        _AreCubesUpright = message.areCubesUpright;
      }
    }

    private void HandleCubeUprightModalClosedByUser() {
      _ReopenModalCooldownStartTimestamp = Time.time;
    }

    private void OpenCubeShouldBeUprightModal() {
      DAS.Debug("PyramidCubeUprightController.OpenCubeShouldBeUprightModal", "Opening modal");
      _ReopenModalCooldownStartTimestamp = kDontCheckModalTimestamp;
      UIManager.OpenModal(_PyramidCubeUprightModalPrefab,
                          new UI.ModalPriorityData(UI.ModalPriorityLayer.High, 0, UI.LowPriorityModalAction.CancelSelf, UI.HighPriorityModalAction.Stack),
                          HandleCubeShouldBeUprightModalCreated);
    }

    private void HandleCubeShouldBeUprightModalCreated(Cozmo.UI.BaseModal newModal) {
      _PyramidCubeUprightModalInstance = (PyramidCubeUprightModal)newModal;
      _PyramidCubeUprightModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished += HandleCubeUprightModalClosedByUser;
    }

    private void HandleSparkComplete(HardSparkEndedByEngine message) {
      CloseCubeShouldBeUprightModal();
    }

    private void CloseCubeShouldBeUprightModal() {
      if (_PyramidCubeUprightModalInstance != null) {
        _PyramidCubeUprightModalInstance.ModalClosedWithCloseButtonOrOutsideAnimationFinished -= HandleCubeUprightModalClosedByUser;
        UIManager.CloseModal(_PyramidCubeUprightModalInstance);
      }
    }
  }
}
