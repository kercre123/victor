using System;
using UnityEngine;
using UnityEngine.UI;
using Cozmo.Util;

namespace Cozmo.UI {
  public class ChallengeBadge : MonoBehaviour {

    // used to figure out which challenge we are displaying
    public ChallengeDataList ChallengeDataList;

    [SerializeField]
    private IconProxy _Icon;

    public void Initialize(string challengeId) {
      var challengeData = ChallengeDataList.ChallengeData.FirstOrDefault(x => x.ChallengeID == challengeId);

      if (challengeData != null) {
        _Icon.SetIcon(challengeData.ChallengeIcon);
      }
    }
  }
}

