using System;
using UnityEngine;
using System.Collections.Generic;

namespace Cozmo.HomeHub {
  public class TimelineView : MonoBehaviour {
    
    public delegate void ButtonClickedHandler(string challengeClicked, Transform buttonTransform);

    public event ButtonClickedHandler OnLockedChallengeClicked;
    public event ButtonClickedHandler OnUnlockedChallengeClicked;
    public event ButtonClickedHandler OnCompletedChallengeClicked;

    public void CloseView() {
      // TODO: Play some close animations before destroying view
      GameObject.Destroy(gameObject);
    }

    public void CloseViewImmediately() {
      GameObject.Destroy(gameObject);
    }

    public void Initialize(Dictionary<string, ChallengeStatePacket> _challengeStatesById) {

    }
  }
}

