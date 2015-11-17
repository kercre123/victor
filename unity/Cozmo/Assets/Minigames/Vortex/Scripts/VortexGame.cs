using UnityEngine;
using System.Collections;

namespace Vortex {

  public class VortexGame : GameBase {

    [SerializeField]
    private VortexPanel _GamePanelPrefab;

    private VortexPanel _GamePanel;

    private StateMachineManager _StateMachineManager = new StateMachineManager();
    private StateMachine _StateMachine = new StateMachine();

    void Start() {

      _StateMachine.SetGameRef(this);
      _StateMachineManager.AddStateMachine("VortexStateMachine", _StateMachine);
      DAS.Info(this, "VortexGame::Start");
      InitialCubesState initCubeState = new InitialCubesState();
      initCubeState.InitialCubeRequirements(new StateIntro(), 1, OnIntroComplete);
      _StateMachine.SetNextState(initCubeState);

      _GamePanel = UIManager.OpenDialog(_GamePanelPrefab).GetComponent<VortexPanel>();
      CreateDefaultQuitButton();

    }

    void Update() {
      _StateMachineManager.UpdateAllMachines();
    }

    public override void CleanUp() {
      
      if (_GamePanel != null) {
        UIManager.CloseDialogImmediately(_GamePanel);
      }
      DestroyDefaultQuitButton();

    }

    public void OnIntroComplete() {
      DAS.Info(this, "VortexGame::OnIntroComplete");
    }
  }

}
