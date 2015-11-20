using UnityEngine;
using UnityEngine.UI;
using System.Collections;

namespace RotationTraining {
  public class RotationTrainingPanel : BaseView {

    [SerializeField]
    private TimeTextLabel _TimerText;

    public void SetTimeLeft(int secondsLeft) {
      _TimerText.SetTimeLeft(secondsLeft);
    }

    protected override void CleanUp() {
    
    }

    protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    
    }
  }
}
