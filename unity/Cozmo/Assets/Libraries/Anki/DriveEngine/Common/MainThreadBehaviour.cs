using UnityEngine;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System;

//
// TODO (BRC) MainThreadBehaviour is deprecated.
// Remove all uses of this class and delete it.
// 

namespace Anki {

  public class MainThreadBehaviour : MonoBehaviour {

    public static void Dispatch(Action action) {
      Anki.DispatchQueue.MainThreadDispatchQueue.Async(action);
    }

    protected void DispatchMain(Action action) {
      Anki.DispatchQueue.MainThreadDispatchQueue.Async(action);
    }

    protected virtual void Update() {
    }

    protected virtual void Awake() {
    }

  }
}

