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
      private int _GoalCurrent;

      [SerializeField]
      private AnkiTextLabel _GoalLabel;

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
            if (OnProgChanged != null) {
              OnProgChanged.Invoke();
            }
          }
        }
      }

      public string GoalLabelText {
        get {
          return _GoalLabel.text;
        }
        set {
          _GoalLabel.text = value;
        }
      }

      public void ResetProgress() {
        _GoalProgressBar.ResetProgress();
        Progress = 0.0f;
      }

      public void SetProgress(float progress) {
        _GoalCurrent = (int)((float)_GoalTarget * progress);
        Progress = progress;
        if (progress >= 1.0f) {
          GoalLabelText = Localization.Get(LocalizationKeys.kDailyGoalComplete);
          _GoalLabel.color = new Color(232.0f, 255.0f, 139.0f);
        }
      }

      public void SetProgress(int progress) {
        if (progress > _GoalTarget) {
          _GoalCurrent = _GoalTarget;
        }
        else {
          _GoalCurrent = progress;
        }
        SetProgress((float)_GoalCurrent / (float)_GoalTarget);
      }

      /// <summary>
      /// Initialize the GoalCell with the target ProgressionStatType, Goal, Current Progress, and Whether or not it should
      /// update OnProgressionStatRecieved. (Set that to false if you intend to use goal cell for history.)
      /// </summary>
      /// <param name="type">Type.</param>
      /// <param name="goal">Goal.</param>
      /// <param name="currProg">Curr prog.</param>
      /// <param name="update">If set to <c>true</c> update.</param>
      public void Initialize(Anki.Cozmo.ProgressionStatType type, int goal, int currProg, bool update = true) {
        GoalLabelText = string.Format("{0}", type.ToString());
        _GoalTarget = goal;
        Type = type;
        if (update) {
          RobotEngineManager.Instance.OnProgressionStatRecieved += OnProgressionStatUpdate;
        }
        _GoalIcon.sprite = ProgressionStatIconMap.Instance.GetIconForStat(type);
        SetProgress(currProg);
      }

      // Hide text while collapsing, show text when expanded
      public void ShowText(bool show) {
        Sequence fadeTween = DOTween.Sequence();
        if (show) { 
          fadeTween.Append(_GoalLabel.DOFade(1.0f, 0.25f));
        }
        else {
          fadeTween.Append(_GoalLabel.DOFade(0.0f, 0.25f));
        }
        fadeTween.Play();
      }

      void OnProgressionStatUpdate(Anki.Cozmo.ProgressionStatType type, int count) {
        if (Type == type) {
          SetProgress(count);
        }
      }

      void OnDestroy() {
        RobotEngineManager.Instance.OnProgressionStatRecieved -= OnProgressionStatUpdate;
      }
    }
  }
}
