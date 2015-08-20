/**
 * File: NetworkManager.cs
 *
 * Author: damjan
 * Created: feb/4/2015
 *
 * Description: simple implementation of tcp server. listens for incoming connections and spawns a new worker thread for each connection.
 *
 *
 * Copyright: Anki, Inc. 2015
 *
 **/
using UnityEditor;
using UnityEngine;
using System;
using System.Collections.Generic;
using System.Collections;
using System.Text;
using System.Net.Sockets;
using System.Net;
using System.Threading;

  
/// <summary>
/// simple implementation of tcp server. listens for incoming connections and spawns a new worker thread for each connection
/// </summary>
class NetworkManager {
  private int _listenPort = 48888;
    
  // Specify local IP address on which RoadViz listens for basestation connection.
  // If listenAddressStr == "", IP on subnet 192.168.0.* will be auto-detected.
  private string _listenAddressStr = ""; //"192.168.0.19"; 
  private TcpListener _tcpServer = null;
  private List<Thread> _threads;
  private bool isActive = false;
    
  public NetworkManager() {
    _threads = new List<Thread>();
  }
    
    
  /// <summary>
  /// parses address string and starts listening
  /// </summary>
  public void StartListening() {
    // Auto-detect local IP
    IPAddress foundIpAddress = IPAddress.Loopback;
    _listenAddressStr = foundIpAddress.ToString();
//    if (_listenAddressStr == "") {
//      IPHostEntry host;
//      host = Dns.GetHostEntry(Dns.GetHostName());
//      foreach (IPAddress ip in host.AddressList) {
//        if (ip.AddressFamily == AddressFamily.InterNetwork && !ip.ToString().Contains("127.0.0.1")) {
//          _listenAddressStr = ip.ToString();
//          foundIpAddress = ip;
//          break;
//        }
//      }
//    }
    try {
      //Debug.Log(String.Format("listening on {0} : {1}", _listenAddressStr, _listenPort));
      IPAddress listenAddress = foundIpAddress;
      _tcpServer = new TcpListener(listenAddress, _listenPort);
      _tcpServer.Start();
    isActive = true;
    }
    catch (Exception e) {
      Debug.Log("SocketException: " + e.ToString());
    }
      
  }
    
  /// <summary>
  /// stops listening
  /// </summary>
  public void StopListening() {
  isActive = false;
  if (_tcpServer != null) {
    _tcpServer.Stop ();
  }
    Debug.Log(String.Format("connection closed {0} : {1}", _listenAddressStr, _listenPort));
  }
    
    
  /// <summary>
  /// Will do a non-blocking check for a connection request, and if
  /// one is found it will launch a thread to handle the connection
  /// This should be called every frame in the game.update
  /// </summary>
  public void CheckForConnection() {
  if (!isActive) {
      return;
  }
    try {
      // If a connection request is pending then launch a new
      // thread to handle it
      if (_tcpServer.Pending()) {
        TcpClient client = _tcpServer.AcceptTcpClient();
        ConnectionHandler connectionHandler =
            new ConnectionHandler(client);
        ThreadStart job = new ThreadStart(connectionHandler.ThreadEntry);
        Thread thread = new Thread(job);
        thread.Start();
        Debug.Log("new connection accepted");
        _threads.Add(thread);
      }
    }
    catch (Exception e) {
      Debug.Log("SocketException: " + e.ToString());
    }
      
  }
            

  /// <summary>
  /// Cleanup!
  /// </summary>
  public void Close() {
    for (int k = 0; k < _threads.Count; ++k) {
      Thread thread = _threads[k];
      thread.Abort();
      thread.Join();
    }
    _threads.Clear();
    StopListening();
  }
}






