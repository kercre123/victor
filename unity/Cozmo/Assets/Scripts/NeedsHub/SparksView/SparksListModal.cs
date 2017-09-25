using Cozmo.UI;
using Cozmo.Challenge;
using System.Collections.Generic;
using UnityEngine;
using Cozmo.Needs.Sparks.UI.CozmoSings;

namespace Cozmo.Needs.Sparks.UI {
  public class SparksListModal : BaseModal {

    [SerializeField]
    private SparkCell _SparksCellPrefab;

    [SerializeField]
    private CozmoSingsSparkCell _CozmoSingsSparkCellPrefab;

    [SerializeField]
    private Transform _SparksListContainer;

    [SerializeField]
    private Transform _GameListContainer;

    [SerializeField]
    private GameCell _GameCellPrefab;

    public void InitializeSparksListModal(List<ChallengeManager.ChallengeStatePacket> minigameData) {
      for (int i = 0; i < minigameData.Count; ++i) {
        if (minigameData[i].Data.IsMinigame) {
          GameObject cellInstance = UIManager.CreateUIElement(_GameCellPrefab, _GameListContainer);
          cellInstance.GetComponent<GameCell>().Initialize(minigameData[i], DASEventDialogName);
        }
      }

      UnlockableInfo cozmoSingsData = UnlockablesManager.Instance.GetUnlockableInfo(Anki.Cozmo.UnlockId.CozmoSings);
      CreateSparkCell(cozmoSingsData, _CozmoSingsSparkCellPrefab.gameObject);

      List<UnlockableInfo> unlockedUnlockInfos = UnlockablesManager.Instance.GetUnlocked(UnlockableType.Action);
      unlockedUnlockInfos.Sort();
      for (int i = 0; i < unlockedUnlockInfos.Count; ++i) {
        if (unlockedUnlockInfos[i].Id.Value != Anki.Cozmo.UnlockId.CozmoSings) {
          CreateSparkCell(unlockedUnlockInfos[i], _SparksCellPrefab.gameObject);
        }
      }

      List<UnlockableInfo> lockedUnlockInfos = UnlockablesManager.Instance.GetAvailableAndLocked(UnlockableType.Action);
      lockedUnlockInfos.Sort();
      for (int i = 0; i < lockedUnlockInfos.Count; ++i) {
        if (lockedUnlockInfos[i].Id.Value != Anki.Cozmo.UnlockId.CozmoSings) {
          CreateSparkCell(lockedUnlockInfos[i], _SparksCellPrefab.gameObject);
        }
      }

      SparksDetailModal.OnSparkGameClicked += HandleStartChallengePressed;
    }

    private void CreateSparkCell(UnlockableInfo unlockData, GameObject prefab) {
      GameObject cellInstance = UIManager.CreateUIElement(prefab, _SparksListContainer);
      cellInstance.GetComponent<SparkCell>().Initialize(unlockData, DASEventDialogName);
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