using UnityEngine;
using UnityEngine.UI;
using System.Collections;
using System.Collections.Generic;
using System;
using Cozmo.HomeHub;
using System.Linq;

namespace DataPersistence {
  public class DataPersistencePane : MonoBehaviour {

    [SerializeField]
    private Button _ResetSaveDataButton;

    [SerializeField]
    private Button _GenerateFakeDataButton;

    [SerializeField]
    private Button _StartNewSessionButton;

    [SerializeField]
    private Text _SessionDays;

    [SerializeField]
    private Button _WinGameButton;

    [SerializeField]
    private Button _LoseGameButton;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;


    private HomeHub GetHomeHub() {
      var go = GameObject.Find("HomeHub(Clone)");
      if (go != null) {
        return go.GetComponent<HomeHub>();
      }
      return null;
    }

    private void TryReloadHomeHub() {
      var homeHub = GetHomeHub();
      if (homeHub != null) {
        homeHub.TestLoadTimeline();
      }
    }

    private void Start() {
      _ResetSaveDataButton.onClick.AddListener(HandleResetSaveDataButtonClicked);
      _GenerateFakeDataButton.onClick.AddListener(HandleGenerateFakeDataButtonClicked);
      _StartNewSessionButton.onClick.AddListener(StartNewSessionButtonClicked);
      _WinGameButton.onClick.AddListener(WinGameButtonClicked);
      _LoseGameButton.onClick.AddListener(LoseGameButtonClicked);
    }

    private void HandleResetSaveDataButtonClicked() {
      // use reflection to change readonly field
      typeof(DataPersistenceManager).GetField("Data").SetValue(DataPersistenceManager.Instance, new SaveData());
      DataPersistenceManager.Instance.Save();
      TryReloadHomeHub();
    }

    private void HandleGenerateFakeDataButtonClicked() {
      GenerateFakeData(_ChallengeDataList.ChallengeData.Select(x => x.ChallengeID).ToArray());
      DataPersistenceManager.Instance.Save();
      TryReloadHomeHub();
    }

    private void StartNewSessionButtonClicked() {

      var field = typeof(TimelineEntryData).GetField("Date");

      int days = -1;
      int.TryParse(_SessionDays.text, out days);
      if (days < 0) {
        DataPersistenceManager.Instance.Data.Sessions.ForEach(x => field.SetValue(x, x.Date.AddDays(days)));
        DataPersistenceManager.Instance.Save();
      }

      TryReloadHomeHub();
    }

    private void GenerateFakeData(string[] challengeIds) {
      DataPersistenceManager.Instance.Data.Sessions.Clear();

      var today = DataPersistenceManager.Today;

      var startDate = today.AddDays(-TimelineView.kGeneratedTimelineHistoryLength);

      for (int i = 0; i < TimelineView.kGeneratedTimelineHistoryLength; i++) {
        var date = startDate.AddDays(i);

        var entry = new TimelineEntryData(date);

        for (int j = 0; j < (int)Anki.Cozmo.ProgressionStatType.Count; j++) {
          var stat = (Anki.Cozmo.ProgressionStatType)j;
          if (UnityEngine.Random.Range(0f, 1f) > 0.6f) {
            int goal = UnityEngine.Random.Range(0, 6);
            int progress = Mathf.Clamp(UnityEngine.Random.Range(0, goal * 2), 0, goal);
            entry.Goals[stat] = goal;
            entry.Progress[stat] = progress;
          }
        }

        int challengeCount = UnityEngine.Random.Range(0, 20);

        for (int j = 0; j < challengeCount; j++) {
          entry.CompletedChallenges.Add(new CompletedChallengeData() {
            ChallengeId = challengeIds[UnityEngine.Random.Range(0, challengeIds.Length)]
          });
        }

        // don't add any days with goal of 0
        if (entry.Goals.Total > 0) {
          DataPersistenceManager.Instance.Data.Sessions.Add(entry);
        }
      }

    }

    private void WinGameButtonClicked() {
      var homeHub = GetHomeHub();
      if (homeHub != null) {
        var minigame = homeHub.MiniGameInstance;
        if (minigame != null) {
          minigame.RaiseMiniGameWin();
        }
      }
    }

    private void LoseGameButtonClicked() {
      var homeHub = GetHomeHub();
      if (homeHub != null) {
        var minigame = homeHub.MiniGameInstance;
        if (minigame != null) {
          minigame.RaiseMiniGameLose();
        }
      }
    }
  }
}