using UnityEngine;
using System.Collections;
using System;

namespace ScriptedSequences {
  
  public interface IAsyncToken {

    bool Success { get; }

    Exception Error { get; }

    bool IsReady { get; }

    void Abort();

    event Action OnAbort;

  }

  public interface IAsyncToken<T> : IAsyncToken {

    T Value { get; }

    void Ready(Action<IAsyncToken<T>> callback);

  }
}