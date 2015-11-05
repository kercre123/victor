/**
 * File: BuildServer
 *
 * Author: damjan
 * Created: 
 *
 * Description: editor extension for build systems
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/

using UnityEditor;
using UnityEngine;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System;

[InitializeOnLoad]
class BuildServer {


  static NetworkManager _networkManager;
  protected static Queue<Action> _syncQueue;


  /// <summary>
  /// Initializes the <see cref="BuildServer"/> class.
  /// </summary>
  static BuildServer() {

    _syncQueue = new Queue<Action>();
    _networkManager = new NetworkManager();
    _networkManager.StartListening();
    EditorApplication.update += Update;
  }


  /// <summary>
  /// Dispatchs the action on the main thread.
  /// </summary>
  /// <param name="action">Action.</param>
  public static void DispatchMain(Action action) {
    lock (_syncQueue) {
      _syncQueue.Enqueue(action);
    }
  }


  /// <summary>
  /// Update this instance. called by the unity editor update loop.
  /// </summary>
  static void Update() {

    // service network
    if (_networkManager != null) {
      _networkManager.CheckForConnection();
    }

    // service any requests to build
    Queue<Action> dispatchQueue;
    lock (_syncQueue) {
      dispatchQueue = new Queue<Action>(_syncQueue);
      _syncQueue.Clear();
    }
    
    while (dispatchQueue.Count > 0) {
      dispatchQueue.Dequeue()();
    }

  }

  /// <summary>
  /// Releases unmanaged resources and performs other cleanup operations before the <see cref="BuildServer"/> is
  /// reclaimed by garbage collection.
  /// </summary>
  ~BuildServer() {
    if (_networkManager != null) {
      _networkManager.StopListening();
      _networkManager = null;
    }
  }

}



