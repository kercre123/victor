using System;
using System.Windows.Forms;

namespace AnimationTool
{
    partial class MainForm
    {
        private void HeadAngleCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cHeadAngle == null || checkBox.Checked != cHeadAngle.Enabled) return;

            if (cHeadAngle.Enabled)
            {
                ActionManager.Do(new DisableChart(cHeadAngle, checkBox));
            }
            else
            {
                ActionManager.Do(new EnableChart(cHeadAngle, checkBox));
            }
        }

        private void LiftHeightCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cLiftHeight == null || checkBox.Checked != cLiftHeight.Enabled) return;

            if (cLiftHeight.Enabled)
            {
                ActionManager.Do(new DisableChart(cLiftHeight, checkBox));
            }
            else
            {
                ActionManager.Do(new EnableChart(cLiftHeight, checkBox));
            }
        }

        private void AudioRobotCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cAudioRobot == null || checkBox.Checked != cAudioRobot.Enabled) return;

            if (cAudioRobot.Enabled)
            {
                ActionManager.Do(new DisableChart(cAudioRobot, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cAudioRobot, checkBox));
            }
        }

        private void AudioDeviceCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cAudioDevice == null || checkBox.Checked != cAudioDevice.Enabled) return;

            if (cAudioDevice.Enabled)
            {
                ActionManager.Do(new DisableChart(cAudioDevice, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cAudioDevice, checkBox));
            }
        }

        private void FaceAnimationImageCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cFaceAnimationImage == null || checkBox.Checked != cFaceAnimationImage.Enabled) return;

            if (cFaceAnimationImage.Enabled)
            {
                ActionManager.Do(new DisableChart(cFaceAnimationImage, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cFaceAnimationImage, checkBox));
            }
        }

        private void FaceAnimationDataCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cFaceAnimationData == null || checkBox.Checked != cFaceAnimationData.Enabled) return;

            if (cFaceAnimationData.Enabled)
            {
                ActionManager.Do(new DisableChart(cFaceAnimationData, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cFaceAnimationData, checkBox));
            }
        }

        private void BodyMotionCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cBodyMotion == null || checkBox.Checked != cBodyMotion.Enabled) return;

            if (cBodyMotion.Enabled)
            {
                ActionManager.Do(new DisableChart(cBodyMotion, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cBodyMotion, checkBox));
            }
        }
    }
}
