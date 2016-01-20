using System;
using Anki.Cozmo.Audio;
using System.ComponentModel;
using DG.Tweening;

namespace ScriptedSequences.Actions {
  public class FadeAudio : ScriptedSequenceAction {

    public enum FadeVolumeChannel {
      Robot,
      Music
    }

    public FadeVolumeChannel VolumeChannel;

    public float TargetVolume;

    public float Duration;

    private SimpleAsyncToken _Token;

    public override ISimpleAsyncToken Act() {
      _Token = new SimpleAsyncToken();

      if (VolumeChannel == FadeVolumeChannel.Robot) {
        Robot currentRobot = RobotEngineManager.Instance.CurrentRobot;
        DOTween.To(() => currentRobot.GetRobotVolume(), x => currentRobot.SetRobotVolume(x), TargetVolume, Duration).OnComplete(() => DoneTween());
      }
      else {
        // TODO: Fade Music Volume here
      }

      return _Token;
    }

    private void DoneTween() {
      _Token.Succeed();
    }
  }
}

