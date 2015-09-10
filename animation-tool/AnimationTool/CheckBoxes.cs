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

        private void FaceAnimationCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cFaceAnimation == null || checkBox.Checked != cFaceAnimation.Enabled) return;

            if (cFaceAnimation.Enabled)
            {
                ActionManager.Do(new DisableChart(cFaceAnimation, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cFaceAnimation, checkBox));
            }
        }

        private void ProceduralFaceCheckBox(object o, EventArgs e)
        {
            CheckBox checkBox = o as CheckBox;

            if (checkBox == null || cProceduralFace == null || checkBox.Checked != cProceduralFace.Enabled) return;

            if (cProceduralFace.Enabled)
            {
                ActionManager.Do(new DisableChart(cProceduralFace, checkBox));
            }
            else
            {
                ActionManager.Do(new Sequencer.EnableChart(cProceduralFace, checkBox));
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
