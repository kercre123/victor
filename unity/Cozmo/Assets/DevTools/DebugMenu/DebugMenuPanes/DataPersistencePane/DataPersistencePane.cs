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
    private Button _ResetRobotDataButton;

    [SerializeField]
    private Button _StartNewSessionButton;

    [SerializeField]
    private Text _SessionDays;

    [SerializeField]
    private ChallengeDataList _ChallengeDataList;


    [SerializeField]
    private InputField _SaveStringInput;

    [SerializeField]
    private Button _SubmitSaveButton;

    [SerializeField]
    private InputField _SkillProfileCurrentLevel;
    [SerializeField]
    private InputField _SkillProfileHighLevel;
    [SerializeField]
    private InputField _SkillRobotHighLevel;
    [SerializeField]
    private Button _SubmitSkillsButton;

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
      _StartNewSessionButton.onClick.AddListener(StartNewSessionButtonClicked);

      _SubmitSaveButton.onClick.AddListener(SubmitSaveButtonClicked);
      _SubmitSkillsButton.onClick.AddListener(SubmitSkillsButtonClicked);
      _ResetRobotDataButton.onClick.AddListener(SubmitResetRobotData);

      _SaveStringInput.text = DataPersistenceManager.Instance.GetSaveJSON();
      InitSkills();
    }

    private void HandleResetSaveDataButtonClicked() {
      // use reflection to change readonly field
      typeof(DataPersistenceManager).GetField("Data").SetValue(DataPersistenceManager.Instance, new SaveData());
      DataPersistenceManager.Instance.StartNewSession();
      DataPersistenceManager.Instance.Save();
      TryReloadHomeHub();
    }

    private void StartNewSessionButtonClicked() {

      int days;
      if (!int.TryParse(_SessionDays.text, out days)) {
        days = 1;
      }
      if (days > 0) {
        DataPersistenceManager.Instance.Data.DefaultProfile.Sessions.ForEach(x => x.Date = x.Date.AddDays(-days));
        DataPersistenceManager.Instance.Save();
      }

      TryReloadHomeHub();
    }


    private void SubmitSaveButtonClicked() {
      DataPersistenceManager.Instance.DebugSave(_SaveStringInput.text);
      TryReloadHomeHub();
    }

    private void InitSkills() {
      int profileCurrLevel;
      int profileHighLevel;
      int robotHighLevel;
      SkillSystem.Instance.GetDebugSkillsForGame(out profileCurrLevel, out profileHighLevel, out robotHighLevel);
      _SkillProfileCurrentLevel.text = profileCurrLevel.ToString();
      _SkillProfileHighLevel.text = profileHighLevel.ToString();
      _SkillRobotHighLevel.text = robotHighLevel.ToString();
    }

    private void SubmitSkillsButtonClicked() {
      SkillSystem.Instance.SetDebugSkillsForGame(int.Parse(_SkillProfileCurrentLevel.text),
        int.Parse(_SkillProfileHighLevel.text), int.Parse(_SkillRobotHighLevel.text));
    }

    private void SubmitResetRobotData() {
      Anki.Debug.DebugConsoleData.Instance.UnityData.HandleResetRobot("DataPersistancePane");
    }

  }
}