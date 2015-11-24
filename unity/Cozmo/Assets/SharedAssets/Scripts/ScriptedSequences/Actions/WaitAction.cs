using System;
using System.Collections;
using UnityEngine;

namespace ScriptedSequences.Actions {
  public class WaitAction : ScriptedSequenceAction {

    /// <summary>
    /// This is not exact, as it will trigger on 
    /// the first frame after the time has been exceeded
    /// </summary>
    public float TimeInSeconds;

    public bool BlockInput;

    public bool AllowSkipAfterDelay;

    public float SkipDelayInSeconds;

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken token = new SimpleAsyncToken();

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

