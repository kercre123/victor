using System;
using System.Collections.Generic;
using UnityEngine;

/*
 * Works with our Java "MessageSender" class to receive messages from Java.
 * When added on a game object, this class will pull messages from Java every frame.
 *
 * C# classes can register callbacks with JavaMessageReceiver to get messages with a
 * particular tag.
 */
public class JavaMessageReceiver : MonoBehaviour {

  public delegate void MessageListener(string[] parameters);

  /*
   * Helper base class - our C# Behaviours can derive from this to get
   * access to a simpler subscription interface. All subscriptions added
   * via RegisterJavaListener() will automatically be removed in OnDestroy().
   */
  public class JavaBehaviour : MonoBehaviour {
    private JavaMessageReceiver _Receiver;
    protected void RegisterJavaListener(JavaMessageReceiver receiver, string tag, MessageListener listener) {
      receiver.RegisterListener(this, tag, listener);
      if (_Receiver != null && _Receiver != receiver) {
        throw new System.ArgumentException("receiver already set");
      }
      _Receiver = receiver;
    }
    protected virtual void ClearJavaListeners() {
      if (_Receiver != null) {
        _Receiver.UnregisterListeners(this);
        _Receiver = null;
      }
    }
    protected virtual void OnDestroy() {
      ClearJavaListeners();
    }
  }

  // Container for message callbacks, also contains a handle provided at registration
  // that will also be used to unregister a callback later
  private class HandleListener {
    public object Handle;
    public MessageListener Listener;
    public HandleListener(object o, MessageListener l) {
      Handle = o;
      Listener = l;
    }
  }

  private AndroidJavaObject _MessageReceiver;
  private Dictionary<string, List<HandleListener>> _Listeners = new Dictionary<string, List<HandleListener>>();

  public void RegisterListener(object handle, string tag, MessageListener listener) {
    List<HandleListener> listeners;
    HandleListener newEntry = new HandleListener(handle, listener);

    if (_Listeners.TryGetValue(tag, out listeners)) {
      listeners.Add(newEntry);
    }
    else {
      listeners = new List<HandleListener>();
      listeners.Add(newEntry);
      _Listeners.Add(tag, listeners);
    }
  }

  public void UnregisterListeners(object handle) {
    List<string> removeTags = new List<string>();
    foreach (var kvp in _Listeners) {
      kvp.Value.RemoveAll(t => t.Handle == handle);
      if (kvp.Value.Count == 0) {
        removeTags.Add(kvp.Key);
      }
    }
    foreach (var removeTag in removeTags) {
      _Listeners.Remove(removeTag);
    }
  }

  private void Awake() {
    _MessageReceiver = new AndroidJavaClass("com.anki.util.MessageSender").CallStatic<AndroidJavaObject>("getReceiver");
  }

  private void Update() {
    while (true) {
      string[] message = _MessageReceiver.Call<string[]>("pullMessage");
      if (message == null || message.Length == 0) {
        break;
      }

      string[] parameters = new string[message.Length - 1];
      System.Array.Copy(message, 1, parameters, 0, message.Length - 1);
      HandleMessage(message[0], parameters);
    }
  }

  private void HandleMessage(string tag, string[] parameters) {
    List<HandleListener> tagListeners;
    if (_Listeners.TryGetValue(tag, out tagListeners)) {
      // Copy to an array because a listener can remove itself during this iteration
      HandleListener[] tempListeners = tagListeners.ToArray();
      foreach (var tuple in tempListeners) {
        tuple.Listener(parameters);
      }
    }
  }

  private void OnDestroy() {
    _MessageReceiver.Call("unregister");
  }

}
