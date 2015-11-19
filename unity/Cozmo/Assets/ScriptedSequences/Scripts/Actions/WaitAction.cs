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

    public override ISimpleAsyncToken Act() {
      SimpleAsyncToken token = new SimpleAsyncToken();

      ScriptedSequenceManager.Instance.BootrapCoroutine(WaitForTime(token));

      return token;
    }

    private IEnumerator WaitForTime(SimpleAsyncToken token)
    {
      float startTime = Time.time;

      while (Time.time - startTime < TimeInSeconds) {
        yield return null;
      }

      token.Succeed();
    }
  }
}

