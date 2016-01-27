using System;
using UnityEditor;
using UnityEngine;

[CustomEditor(typeof(LocalNotificationManager))]
public class LocalNotificationManagerEditor : Editor {

  private int _LastSchedulerIndex = -1;
  private int _LastKeyIndex = -1;

  private int _CurrentSchedulerIndex = -1;

  private string _LocalizedString;
  private string _LocalizedStringFile;

  private static string[] _Hours = { "12", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", "11" };
  private static string[] _Minutes = { "00", "01", "02", "03", "04", "05", "06", "07", "08", "09", 
                                       "10", "11", "12", "13", "14", "15", "16", "17", "18", "19", 
                                       "20", "21", "22", "23", "24", "25", "26", "27", "28", "29", 
                                       "30", "31", "32", "33", "34", "35", "36", "37", "38", "39", 
                                       "40", "41", "42", "43", "44", "45", "46", "47", "48", "49", 
                                       "50", "51", "52", "53", "54", "55", "56", "57", "58", "59" };
  private static string[] _AmPm = { "AM", "PM" };

  public override void OnInspectorGUI() {
    base.OnInspectorGUI();
    LocalNotificationManager lnm = (LocalNotificationManager)target;

    EditorDrawingUtility.DrawListWithIndex("Notifications", lnm.NotificationsToSchedule, DrawNotificationScheduler, i => new LocalNotificationManager.NotificatationScheduler());
  }

  private LocalNotificationManager.NotificatationScheduler DrawNotificationScheduler(LocalNotificationManager.NotificatationScheduler scheduler, int index) {
    _CurrentSchedulerIndex = index;
    EditorGUILayout.BeginVertical();
    scheduler.DelayDays = EditorGUILayout.IntField("Delay Days", scheduler.DelayDays);
    scheduler.EarliestHour = DrawTimeWidget("Earliest Hour", scheduler.EarliestHour);
    scheduler.LatestHour = DrawTimeWidget("Latest Hour", scheduler.LatestHour);

    EditorDrawingUtility.DrawListWithIndex("Message Options", scheduler.LocalizationKeyOptions, DrawLocalizationString, i => string.Empty);
    EditorGUILayout.EndVertical();

    return scheduler;
  }

  private float DrawTimeWidget(string title, float time) {
    int hour = Mathf.FloorToInt(time);
    int minute = Mathf.RoundToInt(60 * (time - hour));

    int ampm = hour / 12;
    hour = hour % 12;

    EditorGUILayout.BeginHorizontal();
    hour = EditorGUILayout.Popup(title, hour, _Hours);
    minute = EditorGUILayout.Popup(minute, _Minutes);
    ampm = EditorGUILayout.Popup(ampm, _AmPm);

    EditorGUILayout.EndHorizontal();
    return hour + (12 * ampm) + (minute / 60f);
  }


  private string DrawLocalizationString(string key, int index) {

    EditorGUILayout.BeginVertical();

    bool lastOpen = _CurrentSchedulerIndex == _LastSchedulerIndex && index == _LastKeyIndex;
    bool toggle = EditorGUILayout.Foldout(lastOpen, key);

    if (toggle) {
      if(!lastOpen) {
        _LastSchedulerIndex = _CurrentSchedulerIndex;
        _LastKeyIndex = index;
        EditorDrawingUtility.InitializeLocalizationString(key, out _LocalizedStringFile, out _LocalizedString);
      }

      EditorDrawingUtility.DrawLocalizationString(ref key, ref _LocalizedStringFile, ref _LocalizedString);
    }
    else if(lastOpen) {
      _LastSchedulerIndex = -1;
      _LastKeyIndex = -1;
    }
    EditorGUILayout.EndVertical();

    return key;
  }

}

