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
    private InputField _NotificationTextInput;

    [SerializeField]
    private Button _SubmitNotificationButton;

    [SerializeField]
    private InputField _NotificationDay;
    [SerializeField]
    private InputField _NotificationMonth;
    [SerializeField]
    private InputField _NotificationYear;
    [SerializeField]
    private InputField _NotificationHour;
    [SerializeField]
    private InputField _NotificationMinute;

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
      _SubmitNotificationButton.onClick.AddListener(SubmitNotificationButtonClicked);

      _SubmitSaveButton.onClick.AddListener(SubmitSaveButtonClicked);
      _SubmitSkillsButton.onClick.AddListener(SubmitSkillsButtonClicked);
      _ResetRobotDataButton.onClick.AddListener(SubmitResetRobotData);
       
      var now = DateTime.Now;
      _NotificationYear.text = now.Year.ToString();
      _NotificationMonth.text = now.Month.ToString();
      _NotificationDay.text = now.Day.ToString();
      _NotificationHour.text = now.Hour.ToString();
      _NotificationMinute.text = now.Minute.ToString();

      _SaveStringInput.text = DataPersistenceManager.Instance.GetSaveJSON();
      InitSkills();
    }

    private void HandleResetSaveDataButtonClicked() {
      // use reflection to change readonly field
      typeof(DataPersistenceManager).GetField("Data").SetValue(DataPersistenceManager.Instance, new SaveData());
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
      // Add anything else we save on the robot here.
      SkillSystem.Instance.DebugEraseStorage();
    }

    private void SubmitNotificationButtonClicked() {
      var year = int.Parse(_NotificationYear.text);

      var month = int.Parse(_NotificationMonth.text);

      var day = int.Parse(_NotificationDay.text);

      var hour = int.Parse(_NotificationHour.text);
      var minute = int.Parse(_NotificationMinute.text);

      var dateTime = new DateTime(year, month, day, hour, minute, 0);

      LocalNotificationManager.ScheduleNotification(dateTime, _NotificationTextInput.text);
    }
  }
}