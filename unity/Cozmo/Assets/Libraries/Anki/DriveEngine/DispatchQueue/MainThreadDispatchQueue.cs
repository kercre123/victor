using System;
using UnityEngine;

// Inspired by: https://raw.githubusercontent.com/neuecc/UniRx/master/Assets/UniRx/Scripts/UnityEngineBridge/MainThreadDispatcher.cs
// commit: e8feade0c6c9880f7c3a6df7c8c61d2e2efde74c
// This is a simplified version since we do not need to support Editor Scripts.

namespace Anki.DispatchQueue {
  public sealed class MainThreadDispatchQueue : MonoBehaviour {
    public enum CullingMode {
      Disabled,
      Self,
      All
    }

    public static CullingMode _CullingMode = CullingMode.Self;

    public static void Async(Action action) {
      _Instance._QueueWorker.Enqueue(action);
    }

    public static void Flush() {
      _Instance._QueueWorker.ExecuteAll(_Instance._UnhandledExceptionCallback);
    }

    ThreadSafeQueueWorker _QueueWorker = new ThreadSafeQueueWorker();
    Action<Exception> _UnhandledExceptionCallback = ex => DAS.Error(typeof(MainThreadDispatchQueue), ex.ToString()); // default

    static MainThreadDispatchQueue _Instance;
    static bool _Initialized;

    [ThreadStatic]
    static object
      _MainThreadToken;

    public static string InstanceName {
      get {
        if (null == _Instance) {
          throw new NullReferenceException("Main Queue is not initialized");
        }
        return _Instance.name;
      }
    }

    public static bool IsInitialized {
      get { return (_Initialized && (_Instance != null)); }
    }

    static MainThreadDispatchQueue Instance {
      get {
        Initialize();
        return _Instance;
      }
    }

    public static void Initialize() {
      if (_Initialized) {
        return;
      }

      MainThreadDispatchQueue queue = null;

      try {
        queue = GameObject.FindObjectOfType<MainThreadDispatchQueue>();
      }
      catch {
        var ex = new Exception("DriveEngine requires a MainThreadDispatchQueue component created on the main thread. Make sure it is added to the scene before calling DriveEngine from a worker thread.");
        UnityEngine.Debug.LogException(ex);
        throw ex;
      }

      if (null == queue) {
        _Instance = new GameObject("MainThreadDispatcher").AddComponent<MainThreadDispatchQueue>();
      }
      else {
        _Instance = queue;
      }

      DontDestroyOnLoad(_Instance);
      _MainThreadToken = new object();
      _Initialized = true;
    }

    void Awake() {
      if (null == _Instance) {
        _Instance = this;
        _MainThreadToken = new object();
        _Initialized = true;
        
        // Added for consistency with Initialize()
        DontDestroyOnLoad(gameObject);
      }
      else {
        if (_CullingMode == CullingMode.Self) {
          DAS.Warn(this, "There is already a MainThreadDispatcher in the scene. Removing myself...");
          // Destroy this dispatcher if there's already one in the scene.
          DestroyDispatcher(this);
        }
        else if (_CullingMode == CullingMode.All) {
          DAS.Warn(this, "There is already a MainThreadDispatcher in the scene. Cleaning up all excess dispatchers...");
          CullAllExcessDispatchers();
        }
        else {
          DAS.Warn(this, "There is already a MainThreadDispatcher in the scene.");
        }
      }
    }

    public static void DestroyDispatcher(MainThreadDispatchQueue queue) {
      if (queue != _Instance) {
        _DestroyDispatcher(queue);
      }
    }

    private static void _DestroyDispatcher(MainThreadDispatchQueue queue) {
      if (null == queue)
        return;

      // Try to remove game object if it's empty
      var components = queue.gameObject.GetComponents<Component>();
      if (queue.gameObject.transform.childCount == 0 && components.Length == 2) {
        if (components[0] is Transform && components[1] is MainThreadDispatchQueue) {
          #if !UNITY_EDITOR
          Destroy(queue.gameObject);
          #else
          DestroyImmediate(queue.gameObject);
          #endif
        }
      }
      else {
        // Remove component
        MonoBehaviour.Destroy(queue);
      }
    }

    public static void DestroyAllInEditor() {
#if UNITY_EDITOR
      MainThreadDispatchQueue queue = null;
      
      try {
        queue = GameObject.FindObjectOfType<MainThreadDispatchQueue>();
      }
      catch {
      }

      if (_Instance != queue) {
        _DestroyDispatcher(_Instance);
      }

      if (null != queue) {
        _DestroyDispatcher(queue);
      }

      _Instance = null;
      _MainThreadToken = null;
      _Initialized = false;
#endif
    }

    public static void CullAllExcessDispatchers() {
      var dispatchers = GameObject.FindObjectsOfType<MainThreadDispatchQueue>();
      for (int i = 0; i < dispatchers.Length; i++) {
        DestroyDispatcher(dispatchers[i]);
      }
    }

    void Update() {
      _QueueWorker.ExecuteAll(_UnhandledExceptionCallback);
    }

  }
} // namespace Anki
