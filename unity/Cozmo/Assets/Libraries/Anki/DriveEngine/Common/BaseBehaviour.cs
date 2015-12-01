using UnityEngine;
using System;
using System.Collections;

namespace Anki {
  public class BaseBehaviour : MonoBehaviour {

    //protected SceneContext Context { get; private set; }

    public static BaseBehaviour CreateInstance() {
      BaseBehaviour instance = new GameObject().AddComponent<BaseBehaviour>();
      return instance;
    }

    public static void Dispatch(Action action) {
      Anki.DispatchQueue.MainThreadDispatchQueue.Async(action);
    }

    protected void DispatchMain(Action action) {
      Anki.DispatchQueue.MainThreadDispatchQueue.Async(action);
    }

    /// <summary>
    /// Unregisters callbacks that may have been subscribed to in RegisterCallbacks.
    /// This method invokes UnregisterCallbacks.
    /// </summary>
    public void DisableCallbacks() {
      UnregisterCallbacks();
    }

    /// <summary>
    /// Registers for callbacks.
    /// This method invokes RegisterCallbacks
    /// </summary>
    public void EnableCallbacks() {
      // TODO(BRC) We really should call UnregisterCallbacks before calling RegisterCallbacks.
      // However, this requires that all implementations be idempotent with no side-effects
      // and do not make any assumptions about state. Until this is true, skip this step.
      // UnregisterCallbacks();
      RegisterCallbacks();
    }

    protected virtual void Awake() {
      //Context = SceneDirector.Instance.Context;
      OnLoad();
    }

    protected virtual void OnEnable() {
      EnableCallbacks();
    }
    
    protected virtual void OnDisable() {
      DisableCallbacks();
    }



    /////////////////////////////////////////////////////////////////////////////////////////////////////////
    // LifeCycle Hooks


    /// <summary>
    /// This is called after Awake is called on BaseBehaviour
    /// </summary>
    protected virtual void OnLoad() {
    }

    /// <summary>
    /// Use this hook to Register all Action call backs
    /// </summary>
    protected virtual void RegisterCallbacks() {
    }

    /// <summary>
    /// Use this hook to Unregister all Action call backs.
    /// This method maybe be called multiple times. It should be idempotent (no side-effects).
    /// </summary>
    protected virtual void UnregisterCallbacks() {
    }
  }
}

