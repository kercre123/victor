using System;
using Anki.Cozmo.Audio;
using System.ComponentModel;
using DG.Tweening;

namespace ScriptedSequences.Actions {
  public class FadeAudio : ScriptedSequenceAction {

    public Anki.Cozmo.Audio.GameParameter.ParameterType VolumeChannel;

    // Value between 0.0 & 1.0
    public float TargetVolume;

    // Time is in seconds
    public float Duration = 0;

    public CurveType Curve = CurveType.Linear;

    private SimpleAsyncToken _Token;

    public override ISimpleAsyncToken Act() {
      _Token = new SimpleAsyncToken();

      // FIXME: This will change when we start using wwise to generate sound for Cozmo
      if (VolumeChannel == Anki.Cozmo.Audio.GameParameter.ParameterType.Robot_Volume) {
        Robot currentRobot = RobotEngineManager.Instance.CurrentRobot;
        DOTween.To(() => currentRobot.GetRobotVolume(), x => currentRobot.SetRobotVolume(x), TargetVolume, Duration).OnComplete(() => DoneTween());
      }
      else {
        int timeInMS = (int)(Duration * 1000);
        AudioClient.Instance.PostParameter(VolumeChannel, TargetVolume, GameObjectType.Invalid, timeInMS, Curve);
        _Token.Succeed();
      }

      return _Token;
    }

    private void DoneTween() {
      _Token.Succeed();
    }
  }
}

