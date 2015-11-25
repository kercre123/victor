using System;
using UnityEngine;

namespace Anki {

  public interface IRunLoopHandler {
    void Update();
  }

  public sealed class RunLoop : MonoBehaviour {
    private static RunLoop _MainThreadRunLoop;
    private static bool _Initialized;
	
    [SerializeField]
    private Action<Exception>
    _UnhandledExceptionCallback = ex => DAS.Error(typeof(RunLoop), ex.ToString()); // default


    [SerializeField]
    private IRunLoopHandler[]
      _Handlers;

    // Expose control variables to Unity Editor
    [SerializeField]
    private bool
      Paused;

    [ThreadStatic]
    private static object
      _MainThreadToken;

    Anki.DispatchQueue.ThreadSafeQueueWorker _QueueWorker = new Anki.DispatchQueue.ThreadSafeQueueWorker();

    public static RunLoop MainThreadRunLoop {
      get {
        Initialize();
        return _MainThreadRunLoop;
      }
    }

    public static void Initialize() {
      if (_Initialized) {
        return;
      }
    
      RunLoop runLoop = null;
    
      try {
        runLoop = GameObject.FindObjectOfType<RunLoop>();
      }
      catch {
        var ex = new Exception("DriveEngine requires a RunLoop component created on the main thread. Make sure it is added to the scene before calling DriveEngine API methods.");
        UnityEngine.Debug.LogException(ex);
        throw ex;
      }
    
      if (null == runLoop) {
        _MainThreadRunLoop = new GameObject("RunLoop").AddComponent<RunLoop>();
      }
      else {
        _MainThreadRunLoop = runLoop;
      }
    
      DontDestroyOnLoad(_MainThreadRunLoop);
      _MainThreadToken = new object();
      _Initialized = true;
      DAS.Debug("RunLoop.Initialize", "");
    }

    public static void DestroyRunLoop(RunLoop runLoop) {
      if (runLoop != _MainThreadRunLoop) {
        _DestroyRunLoop(runLoop);
      }
    }

    private static void _DestroyRunLoop(RunLoop runLoop) {
      if (null == runLoop)
        return;

      // Try to remove game object if it's empty
      var components = runLoop.gameObject.GetComponents<Component>();
      if (runLoop.gameObject.transform.childCount == 0 && components.Length == 2) {
        if (components[0] is Transform && components[1] is RunLoop) {
#if !UNITY_EDITOR
          Destroy(runLoop.gameObject);
#else
          DestroyImmediate(runLoop.gameObject);
#endif
        }
      }
      else {
        // Remove component
        MonoBehaviour.Destroy(runLoop);
      }
    }

    public static void DestroyAllInEditor() {
#if UNITY_EDITOR
      RunLoop runLoop = null;
      
      try {
        runLoop = GameObject.FindObjectOfType<RunLoop>();
      }
      catch {
      }

      if (_MainThreadRunLoop != runLoop) {
        _DestroyRunLoop(_MainThreadRunLoop);
      }

      if (null != runLoop) {
        _DestroyRunLoop(runLoop);
      }

      _MainThreadRunLoop = null;
      _MainThreadToken = null;
      _Initialized = false;
#endif
    }

    public RunLoop() {
      _Handlers = new IRunLoopHandler[0];
    }

    void Awake() {
      if (null == _MainThreadRunLoop) {
        _MainThreadRunLoop = this;
        _MainThreadToken = new object();
        _Initialized = true;
      
        // Added for consistency with Initialize()
        DontDestroyOnLoad(gameObject);
        DAS.Debug("RunLoop.Awake", "");
      }
      else {
        DAS.Warn(this, "There is already a RunLoop attached in the MainThread in the scene. Removing myself...");
        // Destroy this dispatcher if there's already one in the scene.
        DestroyRunLoop(this);
      }
    }

    public void Update() {
      if (Paused) {
        return;
      }

      int handlerCount = _Handlers.Length;
      for (int i = 0; i < handlerCount; ++i) {
        IRunLoopHandler handler = _Handlers[i];
        if (null != handler) {
          Profiler.BeginSample("Handlers");
          handler.Update();
          Profiler.EndSample();
        }
      }

      Profiler.BeginSample("QueueWorkers");
      _QueueWorker.ExecuteAll(_UnhandledExceptionCallback);
      Profiler.EndSample();
    }

    public void Run(int timeoutMilliseconds) {
      int sleepTimeMsec = 4;
      int sleepMsec = 0;
      while (sleepMsec < timeoutMilliseconds) {
        Anki.RunLoop.MainThreadRunLoop.Update();
        System.Threading.Thread.Sleep(sleepTimeMsec);
        sleepMsec += sleepTimeMsec;
      }
    }

    public void PerformAction(Action action) {
      _QueueWorker.Enqueue(action);
    }

    public void AddHandler(IRunLoopHandler handler) {
      int count = _Handlers.Length;
      int newCount = count + 1;

      Array.Resize(ref _Handlers, count + 1);

      _Handlers[count] = handler;

      DAS.Debug("RunLoop.AddHandler", "Count: " + newCount);
    }

    public bool RemoveHandler(IRunLoopHandler handler) {
      int count = _Handlers.Length;

      int index = -1;
      for (int i = 0; i < count; ++i) {
        IRunLoopHandler h = _Handlers[i];
        if (h == handler) {
          index = i;
          break;
        }
      }

      if (index >= 0) {
        // fast delete: move to end of array and resize
        int lastIndex = count - 1;
        if (lastIndex > 0) {
          IRunLoopHandler lastElement = _Handlers[lastIndex];
          _Handlers[index] = lastElement;
        }
        Array.Resize(ref _Handlers, count - 1);
        DAS.Debug("RunLoop.RemoveHandler", "Count: " + (count - 1));
      }

      return (index >= 0);
    }
  }

}
