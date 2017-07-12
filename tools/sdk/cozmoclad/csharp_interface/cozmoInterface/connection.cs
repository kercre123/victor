// Copyright (c) 2016-2017 Anki, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License in the file LICENSE.txt or at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

using System.Collections.Generic;
using System.Linq;
using System.Threading;
using System.IO;

using System.Net;
using System.Net.Sockets;

namespace Anki
{
  namespace Cozmo
  {
    public class Connection
    {
      public UiConnectionType SDKConnectionType = UiConnectionType.SdkOverTcp;

      private const int _MaxBufferSize = 8192;

      private IPEndPoint _endPoint = null;
      private Socket _socket = null;
      private bool _verboseLogging = false; 
      private Thread _listenThread = null;
      private bool _shutdownSignal = false;
      private Dictionary<System.Type, Dictionary<object, iCallback>> _callbacks = new Dictionary<System.Type, Dictionary<object, iCallback>>();
      private List<Action> _inFlightActions = new List<Action>();
      private bool _open = false;
      private byte _currentRobotId = byte.MaxValue;

      public byte CurrentRobotId { get { return _currentRobotId; } }

      public Connection(string ip, int socket, bool verboseLogging = false)
      {
        _verboseLogging = verboseLogging;

        AddCallback<ExternalInterface.Ping>(this, HandlePing);
        AddCallback<ExternalInterface.UiDeviceConnected>(this, HandleUiDeviceConnected);
        AddCallback<ExternalInterface.RobotCompletedAction>(this, HandleActionCompleted);

        System.Console.WriteLine("Connecting to engine");

        _endPoint = new IPEndPoint(IPAddress.Parse(ip), socket);
        _socket = new Socket(_endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
        _socket.SetSocketOption(SocketOptionLevel.Socket, SocketOptionName.ReuseAddress, true);
        _socket.Connect(_endPoint);

        _listenThread = new Thread(PollSocket);
        _listenThread.Start();
        _open = true;

        BlockUntilCondition waitForRobotId = new BlockUntilCondition(() => _currentRobotId != byte.MaxValue);
        waitForRobotId.Block();
      }

      public void Close()
      {
        if (_open)
        {
          _open = false;
          _shutdownSignal = true;
          _listenThread.Join();

          System.Console.WriteLine("Disconnecting from engine");

          _socket.Disconnect(true);
          _socket.Close();
        }
      }

      public Action SendAction<T>(T state, int numRetries = 0, bool inParallel = false)
      {
        Action result = new Action( this, _currentRobotId );
        result.Initialize(state, numRetries, inParallel);

        _inFlightActions.Add(result);

        SendMessage(result.Message);

        return result;
      }

      public void SendMessage<T>(T state)
      {
        SDKMessageOut messageWrapper = new SDKMessageOut();
        messageWrapper.Message.Initialize(state);
        SendMessageInternal(messageWrapper);
      }

      private void SendMessageInternal(SDKMessageOut messageOut)
      {
        if (!_open)
        {
          System.Console.WriteLine("Attempt to send message while not connected - " + messageOut.GetTag());
          return;
        }
        try
        {
          MemoryStream stream = new MemoryStream(_MaxBufferSize);
          BinaryWriter writer = new BinaryWriter(stream);

          writer.Write((short)messageOut.Size);
          messageOut.Pack(stream);

          int packetSize = 2 + messageOut.Size;

          int bytesSent = _socket.Send(stream.GetBuffer(), 0, packetSize, SocketFlags.None);

          if (bytesSent != packetSize)
          {
            System.Console.WriteLine("Wrong number of bytes sent: " + bytesSent.ToString() + ", expected: " + packetSize.ToString());
          }
        }
        catch (System.Exception e)
        {
          System.Console.WriteLine(e.Message);
          Close();
        }
      }

      private void ReceiveMessage(SDKMessageIn messageIn)
      {
        var message = messageIn.Message;

        if (_verboseLogging) { System.Console.WriteLine("Recieved Message - " + message.GetTag()); }

        // since the property to access individual message data in a CLAD message shares its name
        // with the message's tag, we can use that name to get the property that retrieves message
        // data for this type, and execute it on this message
        object messageData = typeof(Anki.Cozmo.ExternalInterface.MessageEngineToGame).GetProperty(message.GetTag().ToString()).GetValue(message, null);

        Dictionary<object, iCallback> callbacks;
        if (_callbacks.TryGetValue(messageData.GetType(), out callbacks))
        {
          iCallback[] callbacksToExectute = callbacks.Values.ToArray();
          foreach (iCallback callback in callbacksToExectute)
          {
            callback.Execute(messageData);
          }
        }

        // does this give us a robot id?
        string robotIDPropertyName = "robotID";
        if (_currentRobotId == byte.MaxValue && messageData.GetType().GetProperty(robotIDPropertyName) != null )
        {
          uint robotId = (uint)messageData.GetType().GetProperty(robotIDPropertyName).GetValue(messageData, null);
          _currentRobotId = (byte)robotId;
          System.Console.WriteLine("Robot connected with id " + _currentRobotId.ToString());
        }
      }

      private bool ParseBuffer(List<System.Byte> buffer)
      {
        if (buffer.Count < 2)
        {
          return false;
        }

        System.Byte[] byteArray = buffer.ToArray();
        MemoryStream stream = new MemoryStream(byteArray);
        BinaryReader br = new BinaryReader(stream);

        int messageSize = (int)br.ReadInt16();
        int packetSize = 2 + messageSize;

        if (stream.Length >= packetSize)
        {
          SDKMessageIn message = null;
          try
          {
            message = new SDKMessageIn();
            try
            {
              message.Unpack(stream);
            }
            catch (System.Exception e)
            {
              if (message.Size != messageSize)
              {
                throw new System.Exception("Could not parse message " + message.GetTag() + ": " +
                  "message size " + message.Size.ToString() +
                  " not equal to buffer size " + messageSize.ToString(),
                  e);
              }
              throw;
            }

            if (message.Size != messageSize)
            {
              throw new System.Exception("Could not parse message " + message.GetTag() + ": " +
                "message size " + message.Size.ToString() +
                " not equal to buffer size " + messageSize.ToString());
            }
          }
          catch (System.Exception e)
          {
            System.Console.WriteLine(e.Message);
            Close();
            return false;
          }

          ReceiveMessage(message);

          buffer.RemoveRange(0, packetSize);
          return true;
        }
        else
        {
          return false;
        }
      }

      private void PollSocket()
      {
        System.Byte[] socketBuffer = new System.Byte[_MaxBufferSize];
        List<System.Byte> recievedBuffer = new List<System.Byte>();

        while (!_shutdownSignal)
        {
          int bytesRead = _socket.Receive(socketBuffer, 0, _MaxBufferSize, SocketFlags.None);
          if (bytesRead > 0)
          {
            if (_verboseLogging) { System.Console.WriteLine("Read " + bytesRead.ToString() + " bytes"); }
            for (int i = 0; i < bytesRead; ++i) { recievedBuffer.Add(socketBuffer[i]); }

            while (ParseBuffer(recievedBuffer)) { }
          }

          // 250 ms sleep
          Thread.Sleep(250);
        }
      }

      public void AddCallback<T>(object obj, System.Action<T> action)
      {
        if (!_callbacks.ContainsKey(typeof(T)))
        {
          _callbacks[typeof(T)] = new Dictionary<object, iCallback>();
        }

        if (_callbacks[typeof(T)].ContainsKey(obj))
        {
          throw new System.Exception("Cannot register the same object multiple times for the same action");
        }

        _callbacks[typeof(T)][obj] = new CallbackImpl<T>(action);
      }

      public void RemoveCallback<T>(object obj)
      {
        if (!_callbacks.ContainsKey(typeof(T)))
        {
          throw new System.Exception("Cannot remove callback, because no callbacks of type " + typeof(T).FullName + " are registered");
        }
        Dictionary<object, iCallback> callbackEntries = _callbacks[typeof(T)];

        if (!callbackEntries.ContainsKey(obj))
        {
          throw new System.Exception("Cannot remove callback, because no callbacks of type " + typeof(T).FullName + " are registered to the object with address " + obj);
        }

        callbackEntries.Remove(obj);
      }

      private void HandlePing(Anki.Cozmo.ExternalInterface.Ping message)
      {
        SendMessage(new Anki.Cozmo.ExternalInterface.Ping(message.counter, message.timeSent_ms, true));
      }

      private void HandleUiDeviceConnected(Anki.Cozmo.ExternalInterface.UiDeviceConnected message)
      {
        // @TODO: Send message "wrong version" - check python SDK
        if (!message.toGameCLADHash.SequenceEqual(MessageEngineToGameHash._Data))
        {
          SendMessage(new ExternalInterface.UiDeviceConnectionWrongVersion(reserved: 0, connectionType: message.connectionType, deviceID: message.deviceID, buildVersion: "?.?.?"));
          throw new System.Exception("CladMismatchEngineToGame - Engine's hash (" +
               System.BitConverter.ToString(message.toGameCLADHash) + ") != UI's (" +
               System.BitConverter.ToString(MessageEngineToGameHash._Data) + ")");
        }

        if (!message.toEngineCLADHash.SequenceEqual(MessageGameToEngineHash._Data))
        {
          SendMessage(new ExternalInterface.UiDeviceConnectionWrongVersion(reserved: 0, connectionType: message.connectionType, deviceID: message.deviceID, buildVersion: "?.?.?"));
          throw new System.Exception("CladMismatchGameToEngine - Engine's hash (" +
               System.BitConverter.ToString(message.toEngineCLADHash) + ") != UI's (" +
               System.BitConverter.ToString(MessageGameToEngineHash._Data) + ")");
        }

        SendMessage(new ExternalInterface.UiDeviceConnectionSuccess(connectionType: message.connectionType, 
                                                                    deviceID: message.deviceID, 
                                                                    buildVersion: "?.?.?",
                                                                    sdkModuleVersion: "?.?.?", 
                                                                    pythonVersion: "N/A", 
                                                                    pythonImplementation: "N/A",
                                                                    osVersion: "?",
                                                                    cpuVersion: "?"));
        
        System.Console.WriteLine("Connected to Cozmo App");
      }

      private void HandleActionCompleted(ExternalInterface.RobotCompletedAction message)
      {
        IEnumerable<Action> matches = _inFlightActions.FindAll(action => action.ID == message.idTag);

        foreach (Action action in matches)
        {
          action.MarkAsComplete();
          _inFlightActions.Remove(action);
        }
        if (_verboseLogging) { System.Console.WriteLine("Action completed " + message.actionType.ToString()); }
      }
    }
  } // namespace Cozmo
} // namespace Anki