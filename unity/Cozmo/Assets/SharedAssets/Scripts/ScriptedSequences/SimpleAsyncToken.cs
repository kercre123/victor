using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace ScriptedSequences {
  public class SimpleAsyncToken : ISimpleAsyncToken {

    public bool Success { get; private set; }

    public bool IsReady { get; private set; }

    public Exception Error { get; private set; }

    private readonly List<Action<ISimpleAsyncToken>> _Callbacks = new List<Action<ISimpleAsyncToken>>();


    public event Action OnAbort;

    public void Abort()
    {
      _Callbacks.Clear();
      if (OnAbort != null) {
        OnAbort();
      }
    }

    public SimpleAsyncToken(){
    }

    public SimpleAsyncToken(bool ready) {
      if (ready) {
        Succeed();
      }
    }

    public void Succeed() {
      if (!IsReady) {
        IsReady = true;
        Success = true;
        InvokeCallbacks();
      }
    }

    public void Fail(Exception error)
    {
      if (!IsReady) {
        IsReady = true;
        Error = error;
        InvokeCallbacks();
      }
    }

    public void Ready(Action<ISimpleAsyncToken> callback)
    {
      if (callback == null) {
        return;
      }

      if (IsReady) {
        callback(this);
      }
      else {
        _Callbacks.Add(callback);
      }
    }

    private void InvokeCallbacks()
    {
      for (int i = 0; i < _Callbacks.Count; i++) {
        _Callbacks[i](this);
      }
      _Callbacks.Clear();
    }

    public static ISimpleAsyncToken Reduce(params ISimpleAsyncToken[] tokens) {
      if (tokens.Length == 0) {
        return new SimpleAsyncToken(true);
      }

      int count = tokens.Length;

      int completeTokens = 0;

      var returnToken = new SimpleAsyncToken();

      foreach (var token in tokens) {
        returnToken.OnAbort += token.Abort;
        token.Ready(x => {
          if (x.Success) {
            completeTokens++;

            if(completeTokens == count)
            {
              returnToken.Succeed();
            }
          }
          else {
            returnToken.Fail(x.Error);
          }

        });
      }

      return returnToken;
    }

    public static ISimpleAsyncToken Reduce(IEnumerable<ISimpleAsyncToken> tokens) {
      return Reduce(tokens.ToArray());
    }

    public static IAsyncToken<ISimpleAsyncToken[]> PessimisticReduce(params ISimpleAsyncToken[] tokens) {
      if (tokens.Length == 0) {
        return new AsyncToken<ISimpleAsyncToken[]>(new ISimpleAsyncToken[0]);
      }

      int count = tokens.Length;

      int completeTokens = 0;

      var returnToken = new AsyncToken<ISimpleAsyncToken[]>();

      foreach (var token in tokens) {
        returnToken.OnAbort += token.Abort;
        token.Ready(x => {
          completeTokens++;

          if(completeTokens == count)
          {
            returnToken.Succeed(tokens);
          }
        });
      }

      return returnToken;
    }

    public static IAsyncToken<ISimpleAsyncToken[]> PessimisticReduce(IEnumerable<ISimpleAsyncToken> tokens) {
      return PessimisticReduce(tokens.ToArray());
    }
  }
}
