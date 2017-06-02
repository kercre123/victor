using Cozmo.UI;
using Cozmo.Challenge;
using System.Collections.Generic;
using UnityEngine;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksListModal : BaseModal {

    [SerializeField]
    private SparkCell _SparksCellPrefab;

    [SerializeField]
    private Transform _SparksListContainer;

    [SerializeField]
    private Transform _GameListContainer;

    [SerializeField]
    private GameCell _GameCellPrefab;

    public void InitializeSparksListModal(List<ChallengeManager.ChallengeStatePacket> minigameData, List<UnlockableInfo> unlockData) {
      for (int i = 0; i < minigameData.Count; ++i) {
        if (minigameData[i].Data.IsMinigame) {
          GameObject cellInstance = UIManager.CreateUIElement(_GameCellPrefab, _GameListContainer);
          cellInstance.GetComponent<GameCell>().Initialize(minigameData[i], DASEventDialogName);
        }
      }

      for (int i = 0; i < unlockData.Count; ++i) {
        if (unlockData[i].FeatureIsEnabled &&
            (UnlockablesManager.Instance.IsUnlocked(unlockData[i].Id.Value) || unlockData[i].ComingSoon)) {
          GameObject cellInstance = UIManager.CreateUIElement(_SparksCellPrefab, _SparksListContainer);
          cellInstance.GetComponent<SparkCell>().Initialize(unlockData[i], DASEventDialogName);
        }
      }

      SparksDetailModal.OnSparkGameClicked += HandleStartChallengePressed;
    }

    protected override void CleanUp() {
      base.CleanUp();
      SparksDetailModal.OnSparkGameClicked -= HandleStartChallengePressed;
    }

    private void HandleStartChallengePressed(string challengeId) {
      CloseDialogImmediately();
    }
  }
}