using System;
using System.Collections.Generic;

public class CallbackManager {
  private Dictionary<Type, CallbackBase> _Callbacks = new Dictionary<Type, CallbackBase>();

  public void AddCallback<T>(Action<T> callback) {
    CallbackBase typeCallbacks;
    if (!_Callbacks.TryGetValue(typeof(T), out typeCallbacks)) {
      typeCallbacks = new CallbackImpl<T>();
      _Callbacks[typeof(T)] = typeCallbacks;
    }
    typeCallbacks.AddCallback(callback);
  }

  public void RemoveCallback<T>(Action<T> callback) {
    CallbackBase typeCallbacks;
    if (_Callbacks.TryGetValue(typeof(T), out typeCallbacks)) {
      typeCallbacks.RemoveCallback(callback);
      if (typeCallbacks.GetCallbackCount() == 0) {
        _Callbacks.Remove(typeof(T));
      }
    }
  }

  public void MessageReceived(object messageData) {
    CallbackBase typeCallbacks;
    if (_Callbacks.TryGetValue(messageData.GetType(), out typeCallbacks)) {
      typeCallbacks.ExecuteCallback(messageData);
    }
  }

  private abstract class CallbackBase {
    public abstract void AddCallback(object callback);
    public abstract void RemoveCallback(object callback);
    public abstract void ExecuteCallback(object param);
    public abstract int GetCallbackCount();
  }

  private class CallbackImpl<T> : CallbackBase {
    Action<T> _Callback;

    public override void AddCallback(object callback) {
      if (!(callback is Action<T>)) {
        DAS.Error("CallbackImpl<" + typeof(T).FullName + ">.AddCallback", "can't add callback of type " + callback.GetType().FullName);
        return;
      }
      _Callback += ((Action<T>)callback);
    }

    public override void RemoveCallback(object callback) {
      if (!(callback is Action<T>)) {
        DAS.Error("CallbackImpl<" + typeof(T).FullName + ">.RemoveCallback", "can't add callback of type " + callback.GetType().FullName);
        return;
      }
      _Callback -= ((Action<T>)callback);
    }

    public override void ExecuteCallback(object param) {
      if (!(param is T)) {
        DAS.Error("CallbackImpl<" + typeof(T).FullName + ">.ExecuteCallback", "can't execute with param of type " + param.GetType().FullName);
        return;
      }
      if (_Callback != null) {
        _Callback((T)param);
      }
    }

    public override int GetCallbackCount() {
      return _Callback == null ? 0 : _Callback.GetInvocationList().Length;
    }
  }
}
