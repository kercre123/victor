using UnityEngine;
using System.Collections;
using System.Collections.Generic;

using System;
using System.Net;
using System.Net.Sockets;
using System.Net.NetworkInformation;
using System.Threading;

public class SOSTCPClient {

  private TcpListener _TcpListener;
  private Byte[] bytes = new Byte[256];
  private NetworkStream _Stream;
  private bool _IsActive = false;

  public Action<string> OnNewSOSLogEntry;
  private List<string> _Messages = new List<string>();

  private readonly object sync = new object();

  public void StartListening() {
    try {
      _IsActive = true;
      _TcpListener = new TcpListener(Dns.GetHostEntry("localhost").AddressList[0], 4444);
      _TcpListener.Start();
      _TcpListener.BeginAcceptTcpClient(new AsyncCallback(HandleConnected), _TcpListener);
    }
    catch (Exception ex) {
      Debug.LogException(ex);
    }
  }

  public void CleanUp() {
    _IsActive = false;
  }

  public void ProcessMessages() {
    lock (sync) {
      if (OnNewSOSLogEntry != null) {
        for (int i = 0; i < _Messages.Count; ++i) {
          OnNewSOSLogEntry(_Messages[i]);
        }
      }
      _Messages.Clear();
    }
  }

  private void HandleConnected(System.IAsyncResult result) {
    try {

      TcpListener listener = (TcpListener)result.AsyncState;
      var client = listener.EndAcceptTcpClient(result);
      Debug.Log("Connected to SOS server!");
      _Stream = client.GetStream();

      int i;
      while ((i = _Stream.Read(bytes, 0, bytes.Length)) != 0 && _IsActive) {
        lock (sync) {
          _Messages.Add(System.Text.Encoding.ASCII.GetString(bytes, 0, i));
        }
      }
    }
    catch (Exception ex) {
      Debug.LogException(ex);
    }

  }
}
