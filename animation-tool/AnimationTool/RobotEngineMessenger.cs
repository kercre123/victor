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
    public class RobotEngineMessenger
    {
        // Can put TimeSpan.Zero if you want to turn it off
        private static readonly TimeSpan FixedRefreshRate = TimeSpan.FromSeconds(.25);
        public static readonly RobotEngineMessenger instance;

        public readonly ConnectionManager ConnectionManager = new ConnectionManager();

        public readonly SendQueue ProceduralFaceQueue;
        public readonly SendQueue HeadAngleQueue;
        public readonly SendQueue LiftHeightQueue;
        public readonly SendQueue SetIdleAnimationQueue;

        static RobotEngineMessenger()
        {
            instance = new RobotEngineMessenger();
        }

        public RobotEngineMessenger()
        {
            ProceduralFaceQueue = ConnectionManager.CreateSendQueue(FixedRefreshRate);
            HeadAngleQueue = ConnectionManager.CreateSendQueue(FixedRefreshRate);
            LiftHeightQueue = ConnectionManager.CreateSendQueue(FixedRefreshRate);
            SetIdleAnimationQueue = ConnectionManager.CreateSendQueue(FixedRefreshRate);
        }

        public void SendAnimation(string animationName)
        {
            ConnectionManager.SendAnimation(animationName);
        }

        public void SendIdleAnimation(string animationName)
        {
            SetIdleAnimation setIdleAnimationMessage = new SetIdleAnimation();
            setIdleAnimationMessage.robotID = 1;
            setIdleAnimationMessage.animationName = animationName;

            MessageGameToEngine message = new MessageGameToEngine();
            message.SetIdleAnimation = setIdleAnimationMessage;
            SetIdleAnimationQueue.Send(message);
        }

        public void SendProceduralFaceMessage(Sequencer.ExtraProceduralFaceData data)
        {
            if (data == null) return;

            SetIdleAnimation setIdleAnimationMessage = new SetIdleAnimation();
            setIdleAnimationMessage.robotID = 1;
            setIdleAnimationMessage.animationName = "_LIVE_";

            DisplayProceduralFace displayProceduralFaceMessage = new DisplayProceduralFace();
            displayProceduralFaceMessage.leftEye = new float[(int)ProceduralEyeParameter.NumParameters];
            displayProceduralFaceMessage.rightEye = new float[(int)ProceduralEyeParameter.NumParameters];
            displayProceduralFaceMessage.robotID = 1;
            displayProceduralFaceMessage.faceAngle = data.faceAngle;

            for (int i = 0; i < displayProceduralFaceMessage.leftEye.Length && i < data.leftEye.Length; ++i)
            {
                displayProceduralFaceMessage.leftEye[i] = data.leftEye[i];
                displayProceduralFaceMessage.rightEye[i] = data.rightEye[i];
            }

            MessageGameToEngine message = new MessageGameToEngine();
            message.DisplayProceduralFace = displayProceduralFaceMessage;
            message.SetIdleAnimation = setIdleAnimationMessage;
            ProceduralFaceQueue.Send(message);
        }

        public void SendHeadAngleMessage(double angle)
        {
            SetHeadAngle headAngleMessage = new SetHeadAngle();
            headAngleMessage.angle_rad = (float)((angle * Math.PI) / 180); // convert to radians
            headAngleMessage.accel_rad_per_sec2 = 2f;
            headAngleMessage.max_speed_rad_per_sec = 5f;

            MessageGameToEngine message = new MessageGameToEngine();
            message.SetHeadAngle = headAngleMessage;
            HeadAngleQueue.Send(message);
        }

        public void SendLiftHeightMessage(double height)
        {
            SetLiftHeight liftHeightMessage = new SetLiftHeight();
            liftHeightMessage.height_mm = (float)height;
            liftHeightMessage.accel_rad_per_sec2 = 5f;
            liftHeightMessage.max_speed_rad_per_sec = 10f;

            MessageGameToEngine message = new MessageGameToEngine();
            message.SetLiftHeight = liftHeightMessage;
            LiftHeightQueue.Send(message);
        }
    }
}
