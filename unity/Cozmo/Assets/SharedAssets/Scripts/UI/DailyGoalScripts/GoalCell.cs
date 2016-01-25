using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;
using System;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    public class GoalCell : MonoBehaviour  {
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
      }

      public void SetProgress(int progress) {
        if (progress > _GoalTarget) {
          _GoalCurrent = _GoalTarget;
        }
        else {
          _GoalCurrent = progress;
        }
        SetProgress((float)_GoalCurrent/(float)_GoalTarget);
      }

      public void Initialize(string name, int goal, int currProg, Anki.Cozmo.ProgressionStatType type = Anki.Cozmo.ProgressionStatType.Count) {
        GoalLabelText = string.Format("+{0} {1}",goal,name);
        _GoalTarget = goal;
        Type = type;
        RobotEngineManager.Instance.OnProgressionStatRecieved += OnProgressionStatUpdate;
        _GoalIcon.sprite = ProgressionStatIconMap.Instance.GetIconForStat(type);
        SetProgress(currProg);
      }

      // Hide text while collapsing, show text when expanded
      public void ShowText(bool show) {
        Sequence fadeTween = DOTween.Sequence();
        if (show) { 
          fadeTween.Append(_GoalLabel.DOFade(1.0f,0.25f));
        }
        else {
          fadeTween.Append(_GoalLabel.DOFade(0.0f,0.25f));
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

      // Use this for initialization
      void Start () {
      
      }
      
      // Update is called once per frame
      void Update () {
      
      }
    }
  }
}
