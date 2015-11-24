using UnityEngine;
using System.Collections;

public class ChallengeDetailsDialog : BaseView {

  public delegate void ChallengeDetailsClickedHandler(string challengeId);

  public event ChallengeDetailsClickedHandler ChallengeStarted;

  public void Initialize(ChallengeData challengeData, Transform challengeButtonTransform) {
  }

  protected override void CleanUp() {
  }

  protected override void ConstructCloseAnimation(DG.Tweening.Sequence closeAnimation) {
    // Reset the camera
  }
}
