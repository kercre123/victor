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

      public Transform GoalCellSource {
        get {
          return _GoalProgressBar.transform;
        }
      }

      public Action<GoalCell> OnProgChanged;

      [SerializeField]
      private AnkiTextLegacy _GoalLabel;

      [SerializeField]
      private Image _CompletedBackground;

      [SerializeField]
      private Image _BoltMark;

      [SerializeField]
      private AnkiTextLegacy _RewardTextLabel;

      [SerializeField]
      private CanvasGroup _CanvasGroup;
      public CanvasGroup CanvasGroup {
        get { return _CanvasGroup; }
      }

      public AnkiTextLegacy GoalLabel {
        get {
          return _GoalLabel;
        }
      }

      private DailyGoal _Goal;
      public DailyGoal Goal {
        get {
          return _Goal;
        }
      }

      private float _GoalProg;

      public float Progress {
        get {
          return _GoalProg;
        }
        set {
          if (_GoalProg != value) {
            _GoalProg = value;
            _GoalProgressBar.SetProgress(value);

            UpdateProgressionUI();

            if (OnProgChanged != null) {
              OnProgChanged(this);
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
        UpdateProgressionUI();
        SetProgress((float)goal.Progress / (float)goal.Target);
        _RewardTextLabel.text = goal.PointsRewarded.ToString();
      }

      public bool GoalComplete() {
        return _GoalProg >= 1.0f;
      }

      private void OnGoalUpdate(DailyGoal goal) {
        SetProgress((float)goal.Progress / (float)goal.Target);
      }

      private void UpdateProgressionUI() {
        _GoalLabel.text = _Goal.Title;

        if (GoalComplete()) {
          _GoalLabel.color = UIColorPalette.CompleteTextColor;
          _CompletedBackground.gameObject.SetActive(true);
          _BoltMark.gameObject.SetActive(false);
          _RewardTextLabel.gameObject.SetActive(false);
        }
        else {
          _GoalLabel.color = UIColorPalette.NeutralTextColor;
          _CompletedBackground.gameObject.SetActive(false);
          _BoltMark.gameObject.SetActive(true);
          _RewardTextLabel.gameObject.SetActive(true);
        }
      }

      void OnDestroy() {
        _Goal.OnDailyGoalUpdated -= OnGoalUpdate;
      }
    }
  }
}
