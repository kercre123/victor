using System;
using System.Collections.Generic;
using System.IO;
using System.Net;
using System.Threading;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo;
using UnityEngine;
using System.Windows.Forms;

namespace AnimationTool
{
    public class SendQueue
    {
        private readonly object sync = new object();
        private readonly ConnectionManager manager;
        private readonly List<MessageGameToEngine> queuedMessages = new List<MessageGameToEngine>();
        private readonly TimeSpan refreshRate;
        private TimeSpan lastExpire;

        public SendQueue(ConnectionManager manager, TimeSpan refreshRate)
        {
            this.manager = manager;
            this.refreshRate = refreshRate;
            this.lastExpire = TimeSpan.FromSeconds(Time.realtimeSinceStartup);
        }

        public void GrabQueuedMessages(List<MessageGameToEngine> destination)
        {
            lock (sync)
            {
                TimeSpan currentTime = TimeSpan.FromSeconds(Time.realtimeSinceStartup);

                if (currentTime - lastExpire < refreshRate) return;

                lastExpire = currentTime;

                destination.AddRange(queuedMessages);
                queuedMessages.Clear();
            }
        }

        public void Clear()
        {
            lock (sync)
            {
                queuedMessages.Clear();
            }
        }

        public void Send(MessageGameToEngine message)
        {
            lock (sync)
            {
                queuedMessages.Clear();
                queuedMessages.Add(message);
            }
            manager.RequestConnect();
        }

        public void Send(IList<MessageGameToEngine> messages)
        {
            lock (sync)
            {
                queuedMessages.Clear();
                queuedMessages.AddRange(messages);
            }
            manager.RequestConnect();
        }
    }

    public class ConnectionManager : IDisposable
    {
        /// <summary>
        /// Occurs on an unspecified thread. May be called even after -=.
        /// </summary>
        private object sync = new object();
        private ChannelBase channel = new UdpChannel();
        private Thread thread = null;
        private bool isStarted = false;
        private bool isStopping = false;
        private static readonly TimeSpan ThreadTick = TimeSpan.FromSeconds(1.0 / 30.0);

        private const int ConnectionRetryCount = 3;
        private int connectionTries = 0;
        private bool connectionRequested = false;
        private static readonly TimeSpan MaxStateMessageWaitTime = TimeSpan.FromSeconds(5);

        private bool resetRequested = false;
        private string queuedAnimationName = null;
        private List<MessageGameToEngine> outgoingQueue = new List<MessageGameToEngine>();
        private List<SendQueue> sendQueues = new List<SendQueue>();
        private MessageGameToEngine idleMessage;
        private bool isReadyToSend = false;
        private int lastStateMessage = 0;

        public event Action<string> ConnectionTextUpdate;
        private string connectionText;
        private bool wasShowingDisconnect = false;

        public string ConnectionText
        {
            get
            {
                lock (sync)
                {
                    return connectionText;
                }
            }
        }

        public ConnectionManager()
        {
            this.connectionText = "[Disconnected]";
        }

        public void Dispose()
        {
            Stop();
        }

        public void Start()
        {
            lock (sync)
            {
                if (isStarted)
                {
                    return;
                }

                isStarted = true;
            }
            thread = new Thread(MainLoop);
            thread.Start();
        }

        public void Stop()
        {
            lock (sync)
            {
                if (isStopping)
                    return;

                isStopping = true;
            }
            thread.Join();
        }

        public void Reset()
        {
            lock (sync)
            {
                resetRequested = true;
            }
        }

        public void RequestConnect()
        {
            lock (sync)
            {
                connectionRequested = true;
                connectionTries = 0;
            }
        }

        public void SendAnimation(string animationName)
        {
            lock (sync)
            {
                queuedAnimationName = animationName;
            }
            RequestConnect();
        }

        public void SetIdleMessage(MessageGameToEngine idleMessage)
        {
            lock (sync)
            {
                this.idleMessage = idleMessage;
                if (!isReadyToSend) return;
            }
            SendMessage(idleMessage);
        }

        public void SendMessage(MessageGameToEngine message)
        {
            lock (sync)
            {
                outgoingQueue.Add(message);
            }
            RequestConnect();
        }

        private void MainLoop()
        {
            StartChannel();
            string currentAnimation = null;
            List<MessageGameToEngine> waitingQueue = new List<MessageGameToEngine>();
            List<MessageGameToEngine> sendQueueMessages = new List<MessageGameToEngine>();
            while (true)
            {
                bool wantReset;
                lock (sync)
                {
                    if (isStopping)
                    {
                        break;
                    }
                    currentAnimation = queuedAnimationName ?? currentAnimation;
                    queuedAnimationName = null;
                    wantReset = resetRequested;
                    resetRequested = false;

                    waitingQueue.AddRange(outgoingQueue);
                    outgoingQueue.Clear();
                }

                if (wantReset)
                {
                    channel.Disconnect();
                    isReadyToSend = false;
                }

                channel.Update();

                if (connectionRequested)
                {
                    if (!EnsureConnected())
                    {
                        lock (sync)
                        {
                            connectionRequested = false;
                            foreach (SendQueue sendQueue in sendQueues)
                            {
                                sendQueue.Clear();
                            }
                        }

                        currentAnimation = null;
                        waitingQueue.Clear();
                    }
                }

                if (channel.IsConnected && isReadyToSend)
                {
                    if (!string.IsNullOrEmpty(currentAnimation))
                    {
                        SendRawAnimation(currentAnimation);
                        currentAnimation = null;
                    }

                    foreach (MessageGameToEngine message in waitingQueue)
                    {
                        channel.Send(message);
                    }
                    waitingQueue.Clear();

                    lock (sync)
                    {
                        foreach (SendQueue sendQueue in sendQueues)
                        {
                            sendQueue.GrabQueuedMessages(sendQueueMessages);
                        }
                    }
                    foreach (MessageGameToEngine message in sendQueueMessages)
                    {
                        Console.WriteLine("sending " + message.GetTag());
                        channel.Send(message);
                    }
                    sendQueueMessages.Clear();
                }

                RaiseConnectionTextUpdate();

                Thread.Sleep(ThreadTick);
            }
            StopChannel();
        }

        private void StartChannel()
        {
            channel.MessageReceived += ReceivedMessage;
            channel.ConnectedToClient += ConnectedToClient;
            channel.DisconnectedFromClient += DisconnectedFromClient;
        }

        private void StopChannel()
        {
            channel.MessageReceived -= ReceivedMessage;
            channel.ConnectedToClient -= ConnectedToClient;
            channel.DisconnectedFromClient -= DisconnectedFromClient;

            channel.Disconnect();
            // wait for disconnect packet to be sent off
            int limit = Environment.TickCount + 2000;
            while (channel.HasPendingOperations)
            {
                if (limit < Environment.TickCount)
                {
                    Console.Write("WARNING: Not waiting for disconnect to finish sending.");
                    break;
                }
                Thread.Sleep(500);
            }
        }

        private bool EnsureConnected()
        {
            if (IsLagging())
            {
                channel.Disconnect();
                isReadyToSend = false;
                RaiseConnectionTextUpdate("Disconnected: State Timed Out");
            }

            if (!channel.IsActive)
            {
                if (connectionTries > ConnectionRetryCount)
                {
                    Console.WriteLine("WARNING: Gave up trying to connect after " + ConnectionRetryCount + " attempts.");
                    return false;
                }

                channel.Connect(RobotSettings.DeviceId, RobotSettings.GetNextFreePort(), RobotSettings.AdvertisingIPAddress, RobotSettings.AdvertisingPort);
                channel.Update();
            }
            return true;
        }

        private void SendRawAnimation(string animationName)
        {
            MessageGameToEngine message = new MessageGameToEngine();
            ReadAnimationFile readAnimationMessage = new ReadAnimationFile();
            message.ReadAnimationFile = readAnimationMessage;
            channel.Send(message);

            PlayAnimation animationMessage = new PlayAnimation();
            animationMessage.robotID = RobotSettings.RobotId;
            animationMessage.animationName = animationName;
            animationMessage.numLoops = 1;

            message = new MessageGameToEngine();
            message.PlayAnimation = animationMessage;
            channel.Send(message);
        }

        private void DisconnectedFromClient(DisconnectionReason reason)
        {
            Console.WriteLine("Disconnected: " + reason.ToString());
            RaiseConnectionTextUpdate("Disconnected: " + reason.ToString());
            isReadyToSend = false;
        }

        private void ConnectedToClient(string connectionID)
        {
            Console.WriteLine("Connected.");
            StartEngine(RobotSettings.VizHostIPAddress);
            ForceAddRobot(RobotSettings.RobotId, RobotSettings.RobotIPAddress, RobotSettings.RobotIsSimulated);
        }

        private void StartEngine(string vizHostIP)
        {
            StartEngine startEngineMessage = new StartEngine();
            startEngineMessage.asHost = 1;
            int length = 0;
            if (!string.IsNullOrEmpty(vizHostIP))
            {
                length = System.Text.Encoding.UTF8.GetByteCount(vizHostIP);
                if (length + 1 > startEngineMessage.vizHostIP.Length)
                {
                    Console.WriteLine("WARNING: vizHostIP is too long. (" + (length + 1).ToString() + " bytes provided, max " + startEngineMessage.vizHostIP.Length + ".)");
                    return;
                }
                System.Text.Encoding.UTF8.GetBytes(vizHostIP, 0, vizHostIP.Length, startEngineMessage.vizHostIP, 0);
            }
            startEngineMessage.vizHostIP[length] = 0;

            MessageGameToEngine message = new MessageGameToEngine();
            message.StartEngine = startEngineMessage;
            channel.Send(message);
        }

        private void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated)
        {
            if (robotID < 0 || robotID > 255)
            {
                Console.WriteLine("WARNING: ID must be between 0 and 255.", "robotID");
                return;
            }

            if (string.IsNullOrEmpty(robotIP))
            {
                Console.WriteLine("WARNING: robotIP is null");
                return;
            }

            ForceAddRobot forceAddRobotMessage = new ForceAddRobot();

            if (System.Text.Encoding.UTF8.GetByteCount(robotIP) + 1 > forceAddRobotMessage.ipAddress.Length)
            {
                Console.WriteLine("WARNING: Robot IP address too long.");
                return;
            }

            int length = System.Text.Encoding.UTF8.GetBytes(robotIP, 0, robotIP.Length, forceAddRobotMessage.ipAddress, 0);
            forceAddRobotMessage.ipAddress[length] = 0;

            forceAddRobotMessage.robotID = (byte)robotID;
            forceAddRobotMessage.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;

            MessageGameToEngine message = new MessageGameToEngine();
            message.ForceAddRobot = forceAddRobotMessage;
            channel.Send(message);
        }

        private void ReceivedMessage(MessageEngineToGame incomingMessage)
        {
            switch (incomingMessage.GetTag())
            {
                case MessageEngineToGame.Tag.RobotAvailable:
                    {
                        MessageGameToEngine message = new MessageGameToEngine();
                        message.ConnectToRobot = new ConnectToRobot();
                        message.ConnectToRobot.robotID = RobotSettings.RobotId;
                        channel.Send(message);
                    }
                    break;
                case MessageEngineToGame.Tag.UiDeviceAvailable:
                    {
                        MessageGameToEngine message = new MessageGameToEngine();
                        message.ConnectToUiDevice = new ConnectToUiDevice();
                        message.ConnectToUiDevice.deviceID = (byte)(incomingMessage.UiDeviceAvailable.deviceID);
                        channel.Send(message);
                    }
                    break;
                case MessageEngineToGame.Tag.RobotState:
                    if (channel.IsConnected)
                    {
                        lastStateMessage = Environment.TickCount;
                        if (!isReadyToSend)
                        {
                            isReadyToSend = true;
                            if (idleMessage != null)
                            {
                                channel.Send(idleMessage);
                            }
                        }
                    }
                    break;
            }
        }

        private bool IsLagging()
        {
            lock (sync)
            {
                return (channel.IsConnected && isReadyToSend && Environment.TickCount - lastStateMessage > MaxStateMessageWaitTime.TotalMilliseconds);
            }
        }

        private void RaiseConnectionTextUpdate()
        {
            RaiseConnectionTextUpdate(null);
        }

        private void RaiseConnectionTextUpdate(string newDisconnectionText)
        {
            string newConnectionText;

            if (IsLagging())
            {
                newConnectionText = "[Lagging]";
            }
            else if (channel.IsConnected && isReadyToSend)
            {
                newConnectionText = "[Connected]";
            }
            else if (channel.IsConnected)
            {
                newConnectionText = "[Waiting for Robot...]";
            }
            else if (channel.IsActive)
            {
                newConnectionText = "[Connecting...]";
            }
            else if (!string.IsNullOrEmpty(newDisconnectionText))
            {
                newConnectionText = newDisconnectionText;
            }
            else if (!wasShowingDisconnect)
            {
                newConnectionText = "[Disconnected]";
            }
            else
            {
                newConnectionText = connectionText;
            }
            wasShowingDisconnect = channel.IsActive;

            if (newConnectionText != connectionText)
            {
                connectionText = newConnectionText;
                Action<string> callback = ConnectionTextUpdate;
                if (callback != null)
                {
                    callback(connectionText);
                }
            }
        }

        public SendQueue CreateSendQueue(TimeSpan refreshRate)
        {
            lock (sync)
            {
                SendQueue channel = new SendQueue(this, refreshRate);
                sendQueues.Add(channel);
                return channel;
            }
        }
    }
}
