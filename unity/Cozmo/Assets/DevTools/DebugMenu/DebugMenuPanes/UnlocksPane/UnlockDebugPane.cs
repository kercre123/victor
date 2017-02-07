using UnityEngine;
using System.Collections;
using System.Collections.Generic;

public class UnlockDebugPane : MonoBehaviour {
  [SerializeField]
  private UnityEngine.UI.Button _LockButton;

  [SerializeField]
  private UnityEngine.UI.Button _UnlockButton;

  [SerializeField]
  private UnityEngine.UI.Dropdown _UnlockSelection;

  [SerializeField]
  private UnityEngine.UI.Button _UnlockDifficultiesButton;

  private void Start() {
    _LockButton.onClick.AddListener(OnHandleLockButtonClicked);
    _UnlockButton.onClick.AddListener(OnHandleUnlockButtonClicked);
    PopulateOptions();

    _UnlockDifficultiesButton.onClick.AddListener(OnHandleUnlockDifficultiesButtonClicked);
  }

  private void PopulateOptions() {
    List<UnityEngine.UI.Dropdown.OptionData> options = new List<UnityEngine.UI.Dropdown.OptionData>();
    for (int i = 0; i < System.Enum.GetValues(typeof(Anki.Cozmo.UnlockId)).Length; ++i) {
      UnityEngine.UI.Dropdown.OptionData option = new UnityEngine.UI.Dropdown.OptionData();
      option.text = System.Enum.GetValues(typeof(Anki.Cozmo.UnlockId)).GetValue(i).ToString();
      options.Add(option);
    }
    _UnlockSelection.AddOptions(options);
  }

  private Anki.Cozmo.UnlockId GetSelectedUnlockId() {
    return (Anki.Cozmo.UnlockId)System.Enum.Parse(typeof(Anki.Cozmo.UnlockId), _UnlockSelection.options[_UnlockSelection.value].text);
  }

  private void OnHandleLockButtonClicked() {
    UnlockablesManager.Instance.TrySetUnlocked(GetSelectedUnlockId(), false);
  }

  private void OnHandleUnlockButtonClicked() {
    UnlockablesManager.Instance.TrySetUnlocked(GetSelectedUnlockId(), true);
  }

  // Difficulties are unlocked in the app not the robot, so not related to real unlock manager
  // but most people logically look for the unlock of any feature here.
  private void OnHandleUnlockDifficultiesButtonClicked() {
    DataPersistence.PlayerProfile playerProfile = DataPersistence.DataPersistenceManager.Instance.Data.DefaultProfile;
    // Set the high scores because in MPspeedtap it checks for "completions" not wins...
    ChallengeData[] challengeList = ChallengeDataList.Instance.ChallengeData;
    for (int i = 0; i < challengeList.Length; ++i) {
      int numDifficulties = challengeList[i].DifficultyOptions.Count;
      playerProfile.GameDifficulty[challengeList[i].ChallengeID] = numDifficulties;
      for (int j = 0; j < numDifficulties; ++j) {
        playerProfile.HighScores[challengeList[i].ChallengeID + j] = 1;
      }
    }
  }

}
