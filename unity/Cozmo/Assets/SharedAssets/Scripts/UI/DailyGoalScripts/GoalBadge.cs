using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;
using System;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    public class GoalBadge : MonoBehaviour  {
      [SerializeField]
      private ProgressBar _GoalProgressBar;

      public Action OnProgChanged;

      private int _GoalTarget;
      private int _GoalCurrent;

      [SerializeField]
      private AnkiTextLabel _GoalLabel;

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

      public void Initialize(string name, int goal, int currProg) {
        GoalLabelText = string.Format("+{0} {1}",goal,name);
        _GoalTarget = goal;
        SetProgress(currProg);
      }

      // Hide text while collapsing, show text when expanded
      public void Expand(bool expand) {
        Sequence fadeTween = DOTween.Sequence();
        if (expand) { 
          fadeTween.Append(_GoalLabel.DOFade(1.0f,0.25f));
        }
        else {
          fadeTween.Append(_GoalLabel.DOFade(0.0f,0.25f));
        }
        fadeTween.Play();
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
