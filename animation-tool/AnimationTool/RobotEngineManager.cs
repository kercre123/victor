using System;
using System.Threading;
using System.Windows.Forms;
using Anki.Cozmo.ExternalInterface;

namespace AnimationTool
{
    class ChannelUpdater
    {
        bool requestStop = false;
        UdpChannel channel;
        public ChannelUpdater(UdpChannel channel_)
        {
            channel = channel_;
        }
        public void Update()
        {
            while (!requestStop)
            {
                channel.Update();
            }
        }

        public void RequestStop()
        {
            requestStop = true;
        }
    }

    public class RobotEngineManager
    {
        string engineIP = "127.0.0.1";
        const int enginePort = 5106;
        const int advertisingRegistrationPort = 5103;
        UdpChannel channel = new UdpChannel();
        MessageGameToEngine Message = new MessageGameToEngine();
        Thread workerThread;
        ChannelUpdater channelUpdater;

        public RobotEngineManager()
        {
            channel.MessageReceived += ReceivedMessage;
            channelUpdater = new ChannelUpdater(channel);
            workerThread = new Thread(channelUpdater.Update);
            workerThread.IsBackground = true;
            workerThread.Start();
            while (!workerThread.IsAlive);
            Thread.Sleep(1);
        }

        public void StopUpdate()
        {
            channelUpdater.RequestStop();
        }

        public void SetEngineIP(string ip)
        {
            engineIP = ip;
        }

        public void Connect()
        {
            if (!channel.IsConnected)
            {
                channel.Connect(1, enginePort, engineIP, advertisingRegistrationPort);
                channel.ConnectedToClient -= ConnectedToClient;
                channel.ConnectedToClient += ConnectedToClient;
            }
        }

        public void SendAnimation(string animationName)
        {
            if (!channel.IsConnected)
            {
                MessageBox.Show("Can only send when connected.", "ERROR", MessageBoxButtons.OK, MessageBoxIcon.Error);
                return;
            }

            ReadAnimationFile readAnimationMessage = new ReadAnimationFile();
            Message.ReadAnimationFile = readAnimationMessage;
            channel.Send(Message);

            PlayAnimation animationMessage = new PlayAnimation();
            animationMessage.robotID = 1;
            animationMessage.animationName = animationName;
            animationMessage.numLoops = 1;

            Message.PlayAnimation = animationMessage;
            channel.Send(Message);
        }

        void ConnectedToClient(string connectionID)
        {
            StartEngine(engineIP);
            ForceAddRobot(1, engineIP, true);
        }

        void ForceAddRobot(int robotID, string robotIP, bool robotIsSimulated)
        {
            if (robotID < 0 || robotID > 255)
            {
                throw new ArgumentException("ID must be between 0 and 255.", "robotID");
            }

            if (string.IsNullOrEmpty(robotIP))
            {
                throw new ArgumentNullException("robotIP");
            }

            ForceAddRobot forceAddRobotMessage = new ForceAddRobot();

            if (System.Text.Encoding.UTF8.GetByteCount(robotIP) + 1 > forceAddRobotMessage.ipAddress.Length)
            {
                throw new ArgumentException("IP address too long.", "robotIP");
            }
            int length = System.Text.Encoding.UTF8.GetBytes(robotIP, 0, robotIP.Length, forceAddRobotMessage.ipAddress, 0);
            forceAddRobotMessage.ipAddress[length] = 0;

            forceAddRobotMessage.robotID = (byte)robotID;
            forceAddRobotMessage.isSimulated = robotIsSimulated ? (byte)1 : (byte)0;

            Message.ForceAddRobot = forceAddRobotMessage;
            channel.Send(Message);
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
                    throw new ArgumentException("vizHostIP is too long. (" + (length + 1).ToString() + " bytes provided, max " + startEngineMessage.vizHostIP.Length + ".)");
                }
                System.Text.Encoding.UTF8.GetBytes(vizHostIP, 0, vizHostIP.Length, startEngineMessage.vizHostIP, 0);
            }
            startEngineMessage.vizHostIP[length] = 0;

            Message.StartEngine = startEngineMessage;
            channel.Send(Message);
        }

        void ReceivedMessage(MessageEngineToGame message)
        {
            switch(message.GetTag())
            {
                case MessageEngineToGame.Tag.RobotAvailable:
                    Message.ConnectToRobot = new ConnectToRobot();
                    Message.ConnectToRobot.robotID = 1;
                    channel.Send(Message);
                    break;
                case MessageEngineToGame.Tag.UiDeviceAvailable:
                    Message.ConnectToUiDevice = new ConnectToUiDevice();
                    Message.ConnectToUiDevice.deviceID = (byte)(message.UiDeviceAvailable.deviceID);
                    channel.Send(Message);
                    break;
            }
        }
    }
}
