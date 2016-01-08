using UnityEngine;
using UnityEngine.UI;
using Cozmo.UI;
using Anki.UI;
using DG.Tweening;
using System.Collections.Generic;

namespace Cozmo {
  namespace UI {
    public class GoalBadge : MonoBehaviour  {
      [SerializeField]
      private ProgressBar _GoalProgressBar;

      private int _GoalTarget;
      private int _GoalCurrent;

      [SerializeField]
      private AnkiTextLabel _GoalLabel;

      [SerializeField]
      private Image _GoalIcon;

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
      }

      public void SetProgress(float progress) {
        _GoalCurrent = (int)((float)_GoalTarget * progress);
        _GoalProgressBar.SetProgress(progress);
      }

      public void SetProgress(int progress) {
        if (progress > _GoalTarget) {
          _GoalCurrent = _GoalTarget;
        }
        else {
          _GoalCurrent = progress;
        }
        _GoalProgressBar.SetProgress((float)_GoalCurrent/(float)_GoalTarget);
      }

      public void Initialize(string name, int goal, int currProg) {
        GoalLabelText = name;
        _GoalTarget = goal;
        SetProgress(currProg);
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
