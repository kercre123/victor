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

      [SerializeField]
      private AnkiTextLabel _GoalLabel;

      public AnkiTextLabel GoalLabel {
        get {
          return _GoalLabel;
        }
      }

      private DailyGoal _Goal;

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
              _GoalLabel.color = UIColorPalette.CompleteTextColor;
            }
            else {
              _GoalLabel.text = _Goal.Title;
              _GoalLabel.color = UIColorPalette.NeutralTextColor;
            }
            if (OnProgChanged != null) {
              OnProgChanged.Invoke();
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

      public void Initialize(DailyGoal goal, bool update = true) {
        _Goal = goal;
        if (update) {
          goal.OnDailyGoalUpdated += OnGoalUpdate;
        }
        // TODO: Load by string? Or set up a config file for mapping icons to enums?
        //_GoalIcon.sprite = goal.GoalIcon;
        _GoalLabel.text = goal.Title;
        SetProgress((float)goal.Progress / (float)goal.Target);
      }

      private void OnGoalUpdate(DailyGoal goal) {
        SetProgress((float)goal.Progress / (float)goal.Target);
      }

      void OnDestroy() {
        _Goal.OnDailyGoalUpdated -= OnGoalUpdate;
      }
    }
  }
}
