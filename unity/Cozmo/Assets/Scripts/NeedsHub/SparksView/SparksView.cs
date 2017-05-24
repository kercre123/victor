using Cozmo.UI;
using Cozmo.Challenge;
using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksView : BaseView {
    public delegate void BackButtonPressedHandler();
    public event BackButtonPressedHandler OnBackButtonPressed;

    [SerializeField]
    private CozmoButton _BackButton;

    [SerializeField]
    private CozmoButton _AskForTrickButton;

    [SerializeField]
    private CozmoButton _AskForGameButton;

    [SerializeField]
    private CozmoButton _ListAbilitesButton;

    [SerializeField]
    private SparksListModal _SparksListModalPrefab;
    private SparksListModal _SparksListModalInstance;

    private List<UnlockableInfo> _AllUnlockData;
    private List<ChallengeManager.ChallengeStatePacket> _MinigameData;

    public void InitializeSparksView(List<ChallengeManager.ChallengeStatePacket> minigameData) {
      _BackButton.Initialize(HandleBackButtonPressed, "back_button", DASEventDialogName);
      _AskForTrickButton.Initialize(HandleAskForTrickButtonPressed, "ask_for_trick", DASEventDialogName);
      _AskForGameButton.Initialize(HandleAskForGameButtonPressed, "ask_for_game", DASEventDialogName);
      _ListAbilitesButton.Initialize(HandleListAbilitiesButtonPressed, "list_abilities_button", DASEventDialogName);
      _MinigameData = minigameData;

      _AllUnlockData = UnlockablesManager.Instance.GetUnlockablesByType(UnlockableType.Action);
      _AllUnlockData.Sort();
    }

    private void HandleBackButtonPressed() {
      if (OnBackButtonPressed != null) {
        OnBackButtonPressed();
      }
    }

    private void HandleAskForTrickButtonPressed() {

    }

    private void HandleAskForGameButtonPressed() {

    }

    private void HandleListAbilitiesButtonPressed() {
      UIManager.OpenModal(_SparksListModalPrefab, new ModalPriorityData(), (obj) => {
        _SparksListModalInstance = (SparksListModal)obj;
        _SparksListModalInstance.InitializeSparksListModal(_MinigameData, _AllUnlockData);
      });
    }

    protected override void CleanUp() {

    }
  }
}
