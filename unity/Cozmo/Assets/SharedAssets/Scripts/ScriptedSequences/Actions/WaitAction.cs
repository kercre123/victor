using System;
using System.Collections;
using UnityEngine;
using System.ComponentModel;

namespace ScriptedSequences.Actions {
  public class WaitAction : ScriptedSequenceAction {

    /// <summary>
    /// This is not exact, as it will trigger on 
    /// the first frame after the time has been exceeded
    /// </summary>
    [Description("The length of time this wait action should wait by default")]
    public float TimeInSeconds;

    [Description("If we should block all input while waiting")]
    public bool BlockInput;

    [Description("This will only work if we block input. "+
                 "Lets the user tap the screen to end this action early after a shorter delay")]
    public bool AllowSkipAfterDelay;

    [Description("The minimum amount of time to wait before allowing the user to tap to continue")]
    public float SkipDelayInSeconds;

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken token = new SimpleAsyncToken();

      token.OnAbort += () => { 
        if(BlockInput)
        {
          UIManager.Instance.HideTouchCatcher();
          token.Succeed();
        }
      };
      ScriptedSequenceManager.Instance.BootstrapCoroutine(WaitForTime(token));

      return token;
    }

    private IEnumerator WaitForTime(SimpleAsyncToken token)
    {
      float startTime = Time.time;

      if (BlockInput) {
        UIManager.Instance.ShowTouchCatcher(() => {
          if(AllowSkipAfterDelay && Time.time - startTime >= SkipDelayInSeconds)
          {
            UIManager.Instance.HideTouchCatcher();
            token.Succeed();
          }
        });
      }

      while (Time.time - startTime < TimeInSeconds) {
        yield return null;
        if (token.IsReady) {
          yield break;
        }
      }

      if (BlockInput) {
        UIManager.Instance.HideTouchCatcher();
      }

      token.Succeed();
    }
  }
}

