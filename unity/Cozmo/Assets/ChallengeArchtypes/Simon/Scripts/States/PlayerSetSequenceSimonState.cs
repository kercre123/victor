using UnityEngine;
using System.Collections;

namespace Simon {
  public class PlayerSetSequenceSimonState : State {

    public override void Enter() {
      base.Enter();
      DAS.Error(this, "Enter");
    }
  }
}