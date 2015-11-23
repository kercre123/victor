using UnityEngine;
using System.Collections;
using System;

namespace ScriptedSequences {
  
  public interface ISimpleAsyncToken : IAsyncToken {
    
    void Ready(Action<ISimpleAsyncToken> callback);
  }
}
