using UnityEditor;
using UnityEngine;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System;
using System.Net.Sockets;
using System.Threading;
using Anki.Network;

namespace Anki {
  namespace Build {

    [InitializeOnLoad]
    class BuildServer {

      private static Anki.Network.TCPServer _networkManager;
      private static Queue<Action> _syncQueue;


      /// <summary>
      /// Initializes the <see cref="BuildServer"/> class.
      /// </summary>
      static BuildServer() {

        _syncQueue = new Queue<Action>();
        _networkManager = new Anki.Network.TCPServer(ConnectionHandlerFactoryMethod, 48888);
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
      private static void Update() {

        // service network
        if (_networkManager != null) {
          _networkManager.CheckForConnection();
        }

        // service any requests to build
        Queue<Action> dispatchQueue = null;
        lock (_syncQueue) {
          if (_syncQueue.Count > 0) {
            dispatchQueue = new Queue<Action>(_syncQueue);
            _syncQueue.Clear();
          }
        }
        
        while ((dispatchQueue != null) && (dispatchQueue.Count > 0)) {
          dispatchQueue.Dequeue()();
        }
      }

      private static ConnectionHandler ConnectionHandlerFactoryMethod() {
        return new BuildServerConnectionHandler();
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
  }
}
