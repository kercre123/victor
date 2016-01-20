using System;
using Anki.Cozmo.Audio;
using System.ComponentModel;
using DG.Tweening;

namespace ScriptedSequences.Actions {
  public class FadeAudio : ScriptedSequenceAction {

    [DefaultValue(true)]
    public bool RobotVolume;

    public float TargetVolume;

    public float Duration;

    private SimpleAsyncToken _Token;

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken _Token = new SimpleAsyncToken();
      DOTween.To(() => RobotEngineManager.Instance.CurrentRobot.GetRobotVolume(), x => RobotEngineManager.Instance.CurrentRobot.SetRobotVolume(x), TargetVolume, Duration).OnComplete(() => DoneTween());
    }

    private void DoneTween() {
      _Token.Succeed();
    }
  }
}

