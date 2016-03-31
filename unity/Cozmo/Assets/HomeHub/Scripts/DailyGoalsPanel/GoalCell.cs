using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;
using System;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    public class GoalCell : MonoBehaviour {
      [SerializeField]
      private ProgressBar _GoalProgressBar;

      public Action OnProgChanged;

      private int _GoalTarget;
      private int _CurrStatVal;

      [SerializeField]
      private AnkiTextLabel _GoalLabel;

      public AnkiTextLabel GoalLabel {
        get {
          return _GoalLabel;
        }
      }

      public Anki.Cozmo.ProgressionStatType Type;

      [SerializeField]
      private Image _GoalIcon;

      private float _GoalProg;

      public float Progress {
        get {
          return _GoalProg;
        }
        set {
          if (_GoalProg != value) {
            _GoalProg = value;
            _GoalProgressBar.SetProgress(value);
            if (_GoalProg >= 1.0f) {
              _GoalLabel.text = Localization.Get(LocalizationKeys.kDailyGoalComplete);
              _GoalLabel.color = UIColorPalette.CompleteTextColor();
            }
            else {
              _GoalLabel.text = ProgressionStatConfig.Instance.GetLocNameForStat(Type);
              _GoalLabel.color = UIColorPalette.NeutralTextColor();
            }
          }
        }
      }

      public void ResetProgress() {
        _GoalProgressBar.ResetProgress();
        Progress = 0.0f;
      }

      public void SetProgress(float progress) {
        if (progress > 1.0f) {
          progress = 1.0f;
        }
        Progress = progress;
      }

      /// <summary>
      /// Initialize the GoalCell with the target ProgressionStatType, Goal, Current Progress, and Whether or not it should
      /// update OnProgressionStatRecieved. (Set that to false if you intend to use goal cell for history.)
      /// </summary>
      /// <param name="type">Type.</param>
      /// <param name="goal">Goal.</param>
      /// <param name="currProg">Curr prog.</param>
      /// <param name="update">If set to <c>true</c> update.</param>
      public void Initialize(Anki.Cozmo.ProgressionStatType type, int currProg, int goal, bool update = true) {
        _GoalTarget = goal;
        Type = type;
        if (update) {
          RobotEngineManager.Instance.OnProgressionStatRecieved += OnProgressionStatUpdate;
        }
        _GoalIcon.sprite = ProgressionStatConfig.Instance.GetIconForStat(type);
        _GoalLabel.text = ProgressionStatConfig.Instance.GetLocNameForStat(type);
        SetProgress((float)currProg / (float)goal);
      }

      void OnProgressionStatUpdate(Anki.Cozmo.ProgressionStatType type, int count) {
        if (Type == type) {
          if (count != _CurrStatVal) {
            _CurrStatVal = count;
            SetProgress((float)count / (float)_GoalTarget);
            if (OnProgChanged != null) {
              OnProgChanged.Invoke();
            }
          }
        }
      }

      void OnDestroy() {
        RobotEngineManager.Instance.OnProgressionStatRecieved -= OnProgressionStatUpdate;
      }
    }
  }
}
