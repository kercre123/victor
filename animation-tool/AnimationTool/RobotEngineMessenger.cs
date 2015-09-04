using System;
using System.IO;
using System.Net;
using System.Threading;
using Anki.Cozmo.ExternalInterface;
using Anki.Cozmo;

namespace AnimationTool
{
    public class RobotEngineMessenger
    {
        /// <summary>
        /// Occurs on an unspecified thread. May be called even after -=.
        /// </summary>
        public event Action<string> ConnectionTextUpdate;
        public string ConnectionText { get; private set; }
        private bool wasShowingDisconnect = false;

        private object sync = new object();
        private Thread thread = null;
        private bool running = false;
        private static readonly TimeSpan ThreadTick = TimeSpan.FromMilliseconds(100);

        private const int ConnectionRetryCount = 3;
        private int connectionTries = 0;
        private static readonly TimeSpan MaxStateMessageWaitTime = TimeSpan.FromSeconds(5);

        private ChannelBase channel = null;
        private MessageGameToEngine message = new MessageGameToEngine();
        private bool resetRequested = false;
        private string queuedAnimationName = null;
        private bool isReadyToSend = false;
        private int lastStateMessage = 0;

        public RobotEngineMessenger()
        {
            this.ConnectionText = "[Disconnected]";
        }

        public void Start()
        {
            running = true;
            thread = new Thread(MainLoop);
            thread.Start();
        }

        public void Stop()
        {
            lock (sync)
            {
                if (!running)
                    return;
                running = false;
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

        public void SendAnimation(string animationName)
        {
            lock (sync)
            {
                queuedAnimationName = animationName;
                connectionTries = 0;
            }
        }

        private void MainLoop()
        {
            StartChannel();
            string currentAnimation = null;
            while (true)
            {
                bool wantReset;
                lock (sync)
                {
                    if (!running)
                    {
                        break;
                    }
                    currentAnimation = queuedAnimationName ?? currentAnimation;
                    queuedAnimationName = null;
                    wantReset = resetRequested;
                    resetRequested = false;
                }

                if (wantReset)
                {
                    channel.Disconnect();
                    isReadyToSend = false;
                }
                channel.Update();
                if (!string.IsNullOrEmpty(currentAnimation) && TrySendAnimation(currentAnimation))
                {
                    currentAnimation = null;
                }
                RaiseConnectionTextUpdate();

                Thread.Sleep(ThreadTick);
            }
            StopChannel();
        }

        private void StartChannel()
        {
            channel = new UdpChannel();
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
                System.Threading.Thread.Sleep(500);
            }
            channel = null;
        }

        private bool TrySendAnimation(string animationName)
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
                    return true;
                }

                channel.Connect(RobotSettings.DeviceId, RobotSettings.GetNextFreePort(), RobotSettings.AdvertisingIPAddress, RobotSettings.AdvertisingPort);
                channel.Update();
            }

            if (channel.IsConnected && isReadyToSend)
            {
                SendRawAnimation(animationName);
                return true;
            }
            return false;
        }

        private void SendRawAnimation(string animationName)
        {
            ReadAnimationFile readAnimationMessage = new ReadAnimationFile();
            message.ReadAnimationFile = readAnimationMessage;
            channel.Send(message);

            PlayAnimation animationMessage = new PlayAnimation();
            animationMessage.robotID = RobotSettings.RobotId;
            animationMessage.animationName = animationName;
            animationMessage.numLoops = 1;

            message.PlayAnimation = animationMessage;
            channel.Send(message);
        }

        private void DisconnectedFromClient(DisconnectionReason reason)
        {
            Console.WriteLine("Disconnected: " + reason.ToString());
            RaiseConnectionTextUpdate("Disconnected: " + reason.ToString());
            isReadyToSend = false;
        }

        void ConnectedToClient(string connectionID)
        {
            Console.WriteLine("Connected.");
            StartEngine(RobotSettings.VizHostIPAddress);
            ForceAddRobot(RobotSettings.RobotId, RobotSettings.RobotIPAddress, RobotSettings.RobotIsSimulated);
        }

        void StartEngine(string vizHostIP)
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

            message.StartEngine = startEngineMessage;
            channel.Send(message);
        }

        void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated)
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

            message.ForceAddRobot = forceAddRobotMessage;
            channel.Send(message);
        }

        void ReceivedMessage(MessageEngineToGame message)
        {
            switch (message.GetTag())
            {
                case MessageEngineToGame.Tag.RobotAvailable:
                    this.message.ConnectToRobot = new ConnectToRobot();
                    this.message.ConnectToRobot.robotID = RobotSettings.RobotId;
                    channel.Send(this.message);
                    break;
                case MessageEngineToGame.Tag.UiDeviceAvailable:
                    this.message.ConnectToUiDevice = new ConnectToUiDevice();
                    this.message.ConnectToUiDevice.deviceID = (byte)(message.UiDeviceAvailable.deviceID);
                    channel.Send(this.message);
                    break;
                case MessageEngineToGame.Tag.RobotState:
                    if (channel.IsConnected)
                    {
                        isReadyToSend = true;
                        lastStateMessage = Environment.TickCount;
                    }
                    break;
            }
        }

        private bool IsLagging()
        {
            return (channel.IsConnected && isReadyToSend && Environment.TickCount - lastStateMessage > MaxStateMessageWaitTime.TotalMilliseconds);
        }

        private void RaiseConnectionTextUpdate()
        {
            RaiseConnectionTextUpdate(null);
        }

        private void RaiseConnectionTextUpdate(string newDisconnectionText)
        {
            if (channel == null) return;

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
                newConnectionText = ConnectionText;
            }
            wasShowingDisconnect = channel.IsActive;

            if (newConnectionText != ConnectionText)
            {
                ConnectionText = newConnectionText;
                Action<string> callback = ConnectionTextUpdate;
                if (callback != null)
                {
                    callback(ConnectionText);
                }
            }
        }

        public void AnimateFace(FaceForm faceForm)
        {
            if (faceForm.Changed && channel.IsConnected)
            {
                DisplayProceduralFace displayProceduralFace = new DisplayProceduralFace();
                displayProceduralFace.leftEye = new float[(int)ProceduralEyeParameter.NumParameters];
                displayProceduralFace.rightEye = new float[(int)ProceduralEyeParameter.NumParameters];

                displayProceduralFace.faceAngle = faceForm.FaceAngle;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.BrowAngle] = faceForm.BrowAngle.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.BrowAngle] = faceForm.BrowAngle.Value;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.BrowShiftY] = faceForm.BrowY.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.BrowShiftY] = faceForm.BrowY.Value;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.EyeHeight] = faceForm.EyeHeight.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.EyeHeight] = faceForm.EyeHeight.Value;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.EyeWidth] = faceForm.EyeWidth.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.EyeHeight] = faceForm.EyeWidth.Value;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.PupilHeightFraction] = faceForm.PupilY.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.PupilHeightFraction] = faceForm.PupilY.Value;
                displayProceduralFace.leftEye[(int)ProceduralEyeParameter.PupilWidthFraction] = faceForm.PupilX.Key;
                displayProceduralFace.rightEye[(int)ProceduralEyeParameter.PupilWidthFraction] = faceForm.PupilX.Value;

                message.DisplayProceduralFace = displayProceduralFace;
                channel.Send(message);
                faceForm.Changed = false;
            }
        }
    }
}
