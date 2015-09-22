using System;
using System.Windows.Forms;

namespace AnimationTool
{
    partial class MainForm
    {
        private void HeadAngleCheckBox(object o, EventArgs e)
        {
            if (headAngle.checkBox == null || headAngle.chart == null || headAngle.checkBox.Checked != headAngle.chart.Enabled) return;

            if (headAngle.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(headAngle));
            }
            else
            {
                ActionManager.Do(new EnableChart(headAngle));
            }
        }

        private void LiftHeightCheckBox(object o, EventArgs e)
        {
            if (liftHeight.checkBox == null || liftHeight.chart == null || liftHeight.checkBox.Checked != liftHeight.chart.Enabled) return;

            if (liftHeight.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(liftHeight));
            }
            else
            {
                ActionManager.Do(new EnableChart(liftHeight));
            }
        }

        private void AudioRobotCheckBox(object o, EventArgs e)
        {
            if (audioRobot.checkBox == null || audioRobot.chart == null || audioRobot.checkBox.Checked != audioRobot.chart.Enabled) return;

            if (audioRobot.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(audioRobot));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(audioRobot));
            }
        }

        private void AudioDeviceCheckBox(object o, EventArgs e)
        {
            if (audioDevice.checkBox == null || audioDevice.chart == null || audioDevice.checkBox.Checked != audioDevice.chart.Enabled) return;

            if (audioDevice.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(audioDevice));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(audioDevice));
            }
        }

        private void FaceAnimationCheckBox(object o, EventArgs e)
        {
            if (faceAnimation.checkBox == null || faceAnimation.chart == null || faceAnimation.checkBox.Checked != faceAnimation.chart.Enabled) return;

            if (faceAnimation.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(faceAnimation));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(faceAnimation));
            }
        }

        private void ProceduralFaceCheckBox(object o, EventArgs e)
        {
            if (proceduralFace.checkBox == null || proceduralFace.chart == null || proceduralFace.checkBox.Checked != proceduralFace.chart.Enabled) return;

            if (proceduralFace.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(proceduralFace));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(proceduralFace));
            }
        }

        private void BodyMotionCheckBox(object o, EventArgs e)
        {
            if (bodyMotion.checkBox == null || bodyMotion.chart == null || bodyMotion.checkBox.Checked != bodyMotion.chart.Enabled) return;

            if (bodyMotion.chart.Enabled)
            {
                ActionManager.Do(new DisableChart(bodyMotion));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(bodyMotion));
            }
        }
    }
}
