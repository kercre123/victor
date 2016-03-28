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
  private Byte[] _Bytes = new Byte[1024];
  private NetworkStream _Stream;
  private bool _IsActive = false;

  public Action<string> OnNewSOSLogEntry;
  private List<string> _Messages = new List<string>();

  private readonly object _Sync = new object();

  public void StartListening() {
    if (_IsActive) {
      DAS.Warn(this, "already listening");
      return;
    }
    try {
      _IsActive = true;
      _TcpListener = new TcpListener(IPAddress.Parse("127.0.0.1"), 4444);
      _TcpListener.Start();
      var result = _TcpListener.BeginAcceptTcpClient(new AsyncCallback(HandleConnected), _TcpListener);
      result.AsyncWaitHandle.WaitOne(TimeSpan.FromSeconds(1));
      DAS.Debug(this, "SOS Server Started Listening");
    }
    catch (Exception ex) {
      Debug.LogException(ex);
    }
  }

  public void CleanUp() {
    _IsActive = false;
  }

  public void ProcessMessages() {
    lock (_Sync) {
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

      // using Debug.Log because DAS log will cause threading issues with updating UI.
      Debug.Log("SOS Server Connected!");
      _Stream = client.GetStream();

      int i;
      while ((i = _Stream.Read(_Bytes, 0, _Bytes.Length)) != 0 && _IsActive) {
        lock (_Sync) {
          // the replacement is so c# strings don't get messed up by nulls in the middle of strings.
          // C# strings do not expect to be null terminated and causes weirdness down the line if they are.
          string logOutput = System.Text.Encoding.ASCII.GetString(_Bytes, 0, i).Replace('\0', ' ');
          logOutput = logOutput.Replace('\n', ' ');
          _Messages.Add(logOutput);
        }
      }
    }
    catch (Exception ex) {
      Debug.LogException(ex);
    }

  }
}
