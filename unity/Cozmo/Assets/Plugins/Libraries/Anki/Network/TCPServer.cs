using UnityEngine;
using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Net.Sockets;
using System.Net;
using System.Threading;

  
namespace Anki {
  namespace Network {

    public delegate ConnectionHandler ConnectionHandlerFactoryMethod();

    /// <summary>
    /// simple implementation of tcp server. listens for incoming connections and spawns a new worker thread for each connection
    /// </summary>
    public class TCPServer {

      private int _ListenPort = 48888;
      private string _ListenAddressStr = ""; 
      private TcpListener _TCPListener = null;
      private List<Thread> _Threads;
      private ConnectionHandlerFactoryMethod _FactoryMethod;
      private bool _IsActive = false;

      public TCPServer(ConnectionHandlerFactoryMethod factoryMethod, int listenPort) {
        _ListenPort = listenPort;
        _Threads = new List<Thread>();
        _FactoryMethod = factoryMethod;
      }

        
      /// <summary>
      /// parses address string and starts listening
      /// </summary>
      public void StartListening() {

        // Auto-detect local IP
        IPAddress foundIpAddress = IPAddress.Loopback;
        _ListenAddressStr = foundIpAddress.ToString();
        try {
          IPAddress listenAddress = foundIpAddress;
          _TCPListener = new TcpListener(listenAddress, _ListenPort);
          _TCPListener.Start();
          _IsActive = true;
        }
        catch (Exception e) {
          DAS.Debug(this, "SocketException: " + e.ToString());
        }
          
      }

      /// <summary>
      /// stops listening
      /// </summary>
      public void StopListening() {
        _IsActive = false;
        if (_TCPListener != null) {
          _TCPListener.Stop();
        }
        DAS.Debug(this, String.Format("connection closed {0} : {1}", _ListenAddressStr, _ListenPort));
      }

        
      /// <summary>
      /// Will do a non-blocking check for a connection request, and if
      /// one is found it will launch a thread to handle the connection
      /// This should be called every frame in the game.update
      /// </summary>
      public void CheckForConnection() {
        if (!_IsActive) {
          return;
        }
        try {
          // If a connection request is pending then launch a new
          // thread to handle it
          if (_TCPListener.Pending()) {
            TcpClient client = _TCPListener.AcceptTcpClient();
            ConnectionHandler connectionHandler = _FactoryMethod();
            connectionHandler.Client = client;
            ThreadStart job = new ThreadStart(connectionHandler.HandleConnection);
            Thread thread = new Thread(job);
            thread.Start();
            DAS.Debug(this, "new connection accepted");
            _Threads.Add(thread);
          }
        }
        catch (Exception e) {
          DAS.Debug(this, "SocketException: " + e.ToString());
        }
          
      }


      /// <summary>
      /// Cleanup!
      /// </summary>
      public void Close() {
        for (int k = 0; k < _Threads.Count; ++k) {
          Thread thread = _Threads[k];
          thread.Abort();
          thread.Join();
        }
        _Threads.Clear();
        StopListening();
      }
    }
  }
}
