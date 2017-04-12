using System;
using UnityEngine;

namespace InvestorDemo {
  public class InvestorDemoConfig : MinigameConfigBase {
    public override int NumCubesRequired() {
      return 1;
    }

    public override int NumPlayersRequired() {
      return 1;
    }

    public string SequenceName;
    public bool UseSequence;
    public Anki.Cozmo.BehaviorChooserType BehaviorChooser;
  }
}

