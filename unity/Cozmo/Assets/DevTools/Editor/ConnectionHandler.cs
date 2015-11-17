/**
 * File: ConnectionHandler.cs
 *
 * Author: damjan
 * Created: feb/4/2015
 *
 * Description: Processes TCP connection and stores data in global queue
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
using System.IO;
using System.Net;
using System.Threading;

/// <summary>
/// Processes TCP connection and stores data in global queue
/// </summary>
class ConnectionHandler {
  public const int kMaxMessageSize = 5000;
  private TcpClient _client;
  string buildResult = null;

  public ConnectionHandler(TcpClient client) {
    _client = client;
  }
    
  // The TCP connection is started by the main thread at this point.
  // just read the stream.
  /// <summary>
  /// Accepts input string
  /// Passes that string to the ProjectBuilder
  /// Sends build result back to the client
  /// </summary>
  public void ThreadEntry() {
        
    try {

      // Get a stream object for reading and writing
      NetworkStream stream = _client.GetStream();
      int numReadTotal = 0;
      Byte[] buffer = new Byte[kMaxMessageSize];
            
      while (stream.CanRead) {
        int numRead = stream.Read(buffer, numReadTotal, (int)buffer.Length - numReadTotal);
        numReadTotal += numRead;
        DAS.Debug("ConnectionHandler", String.Format("received bytes {0} total {1}", numRead, numReadTotal));
        if (numRead > 0) // for now all data gets in at once. in the future, we might have to indicate "end of data"
          break;
      } // while

      // parse message
      string message = System.Text.Encoding.Default.GetString(buffer);
      DAS.Debug("ConnectionHandler", "message " + message);
            
      // run build
      BuildServer.DispatchMain(() => {
        ProjectBuilder builder = new ProjectBuilder();
        string result = builder.Build(message);
        this.BuildCompleted(result);
      });

      // wait for the build to complete
      while (buildResult == null) {
        Thread.Sleep(50);
      }

      // send back status
      // ProjectBuilder returns a non-empty string if the build fails.
      // Default to Success and handle the failure case below.

      byte returnCode = 0;
      string response = "[SUCCESS] build completed\n";

      if (buildResult.Length > 0) {
        // FAILED!
        returnCode = 1;
        response = "[ERROR] build failed\n" + buildResult;
      }

      // Encode the response to send back.
      // Format [ responseCode, ...message... ]
      int resultBufferLen = Encoding.UTF8.GetByteCount(response);
      byte[] resultBuffer = new byte[resultBufferLen + 1];

      resultBuffer[0] = returnCode;
      Encoding.UTF8.GetBytes(response, 0, response.Length, resultBuffer, 1);
      stream.Write(resultBuffer, 0, resultBuffer.Length);

      buildResult = null;
    }  // try
    catch (Exception e) {
      DAS.Debug("ConnectionHandler", "Exception: " + e.ToString());
    }
    finally {
      _client.Close();
    }
  }


  /// <summary>
  /// called on the main thread when the build is complete
  /// </summary>
  /// <param name="result">output of the build command</param>
  public void BuildCompleted(string result) {
    buildResult = result;
  }
   
}

