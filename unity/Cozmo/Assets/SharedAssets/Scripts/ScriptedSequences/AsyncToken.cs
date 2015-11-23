using UnityEngine;
using System.Collections;
using System;
using System.Collections.Generic;
using System.Linq;

namespace ScriptedSequences {
  public class AsyncToken<T> : IAsyncToken<T> {

    public bool Success { get; private set; }

    public T Value { get; private set; }

    public bool IsReady { get; private set; }

    public Exception Error { get; private set; }

    private readonly List<Action<IAsyncToken<T>>> _Callbacks = new List<Action<IAsyncToken<T>>>();

    public event Action OnAbort;

    public void Abort()
    {
      _Callbacks.Clear();
      if (OnAbort != null) {
        OnAbort();
      }
    }

    public AsyncToken(){
    }

    public AsyncToken(T value) {
      Succeed(value);
    }

    public void Succeed(T value) {
      if (!IsReady) {
        IsReady = true;
        Success = true;
        Value = value;
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

    public void Ready(Action<IAsyncToken<T>> callback)
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

    public static IAsyncToken<T[]> Reduce(params IAsyncToken<T>[] tokens) {
      if (tokens.Length == 0) {
        return new AsyncToken<T[]>(new T[0]);
      }

      int count = tokens.Length;

      int completeTokens = 0;

      var returnToken = new AsyncToken<T[]>();

      foreach (var token in tokens) {
        returnToken.OnAbort += token.Abort;
        token.Ready(x => {
          if (x.Success) {
            completeTokens++;

            if(completeTokens == count)
            {
              returnToken.Succeed(tokens.Select(t => t.Value).ToArray());
            }
          }
          else {
            returnToken.Fail(x.Error);
          }

        });
      }

      return returnToken;
    }
      
    public static IAsyncToken<T[]> Reduce(IEnumerable<IAsyncToken<T>> tokens) {
      return Reduce(tokens.ToArray());
    }

    public static IAsyncToken<IAsyncToken<T>[]> PessimisticReduce(params IAsyncToken<T>[] tokens) {
      if (tokens.Length == 0) {
        return new AsyncToken<IAsyncToken<T>[]>(new IAsyncToken<T>[0]);
      }

      int count = tokens.Length;

      int completeTokens = 0;

      var returnToken = new AsyncToken<IAsyncToken<T>[]>();

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

    public static IAsyncToken<IAsyncToken<T>[]> PessimisticReduce(IEnumerable<IAsyncToken<T>> tokens) {
      return PessimisticReduce(tokens.ToArray());
    }

  }
}
