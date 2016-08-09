using UnityEngine;
using System;
using System.Text;
using System.Threading;
using System.Net.Sockets;
using Anki.Network;

namespace Anki {
  namespace Build {
    public class BuildServerConnectionHandler : ConnectionHandler {

      // Entry point for the thread code that is going to handle a connection
      protected override void HandleConnectionImpl() {
        const int kMaxMessageSize = 5000;

        string buildResult = null;

        try {

          // Get a stream object for reading and writing
          NetworkStream stream = Client.GetStream();
          int numReadTotal = 0;
          Byte[] buffer = new Byte[kMaxMessageSize];

          while (stream.CanRead) {
            int numRead = stream.Read(buffer, numReadTotal, (int)buffer.Length - numReadTotal);
            numReadTotal += numRead;
			Debug.Log(String.Format("received bytes {0} total {1}", numRead, numReadTotal));
            if (numRead > 0) // for now all data gets in at once. in the future, we might have to indicate "end of data"
              break;
          }

          // parse message
          string message = System.Text.Encoding.Default.GetString(buffer);
          message = message.TrimEnd('\r', '\n', '\0');
          Debug.Log("message [" + message + "]");

          string[] args = message.Split(' ');

          // run build
          BuildServer.DispatchMain(() => {
            buildResult = Builder.BuildWithArgs(args);
          });

          // wait for the build to complete
          while (buildResult == null) {
            Thread.Sleep(50);
          }

          // send back status
          // Builder returns a non-empty string if the build fails.
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
        }
        catch (Exception e) {
          DAS.Debug(this, "Exception: " + e.ToString());
        }
      }
    }
  }
}