using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.IO;
using Newtonsoft.Json;

namespace AnimationTool
{
    partial class MainForm
    {
        private double GetTimeNeededFromFile(PointData current)
        {
            if (current == null) return -1;

            double x = Math.Round((current.triggerTime_ms + current.durationTime_ms) * 0.001D, 1);

            return x;
        }

        private void ReadAudioRobotFromFile(AudioRobotPointData current)
        {
            if (current == null) return;

            string path = Sequencer.ExtraAudioData.FullPath + current.pathFromRoot;
            double volume = Math.Round(current.volume, 1);
            Sequencer.ExtraAudioData extraData = new Sequencer.ExtraAudioData(path, Math.Round(current.durationTime_ms * 0.001D, 1), volume < MoveSelectedDataPoints.DELTA_X ? 1 : volume);

            if (extraData.Length < MoveSelectedDataPoints.DELTA_X)
            {
                return;
            }

            ActionManager.Do(new Sequencer.EnableChart(audioRobot), true);
            ActionManager.Do(new Sequencer.AddDataPoint(audioRobot.chart, extraData, current.triggerTime_ms * 0.001D, false), true);
        }

        private void ReadAudioDeviceFromFile(AudioDevicePointData current)
        {
            if (current == null) return;

            string path = Sequencer.ExtraAudioData.FullPath + current.pathFromRoot;
            double volume = Math.Round(current.volume, 1);
            Sequencer.ExtraAudioData extraData = new Sequencer.ExtraAudioData(path, Math.Round(current.durationTime_ms * 0.001D, 1), volume < MoveSelectedDataPoints.DELTA_X ? 1 : volume);

            if (extraData.Length < MoveSelectedDataPoints.DELTA_X)
            {
                return;
            }

            ActionManager.Do(new Sequencer.EnableChart(audioDevice), true);
            ActionManager.Do(new Sequencer.AddDataPoint(audioDevice.chart, extraData, current.triggerTime_ms * 0.001D, false), true);
        }

        private void ReadLiftHeightFromFile(LiftHeightPointData current, LiftHeightPointData next)
        {
            if (current == null) return;

            ActionManager.Do(new EnableChart(liftHeight), true);

            if (next == null) // if last point, move to correct y
            {
                DataPoint last = liftHeight.chart.Series[0].Points[liftHeight.chart.Series[0].Points.Count - 1];
                double x = liftHeight.chart.ChartAreas[0].AxisX.Maximum;

                ActionManager.Do(new MoveDataPoint(last, x, current.height_mm), true);
            }
            else
            {
                ActionManager.Do(new AddDataPoint(liftHeight.chart, next.triggerTime_ms * 0.001D, current.height_mm, false), true);
            }
        }

        private void ReadFaceAnimationImageFromFile(FaceAnimationPointData current)
        {
            if (current == null) return;

            string path = Sequencer.ExtraFaceAnimationData.FullPath + current.pathFromRoot;
            Sequencer.ExtraFaceAnimationData extraData = new Sequencer.ExtraFaceAnimationData(path, Math.Round(current.durationTime_ms * 0.001D, 1));

            if(extraData.Length < MoveSelectedDataPoints.DELTA_X)
            {
                return;
            }

            ActionManager.Do(new FaceAnimation.EnableChart(faceAnimation), true);
            ActionManager.Do(new Sequencer.AddDataPoint(faceAnimation.chart, extraData, current.triggerTime_ms * 0.001D, false), true);
        }

        private void ReadProceduralFaceFromFile(ProceduralFacePointData current)
        {
            if (current == null) return;

            Sequencer.ExtraProceduralFaceData extraData = new Sequencer.ExtraProceduralFaceData(current);

            ActionManager.Do(new Sequencer.EnableChart(proceduralFace), true);
            ActionManager.Do(new Sequencer.AddDataPoint(proceduralFace.chart, extraData, current.triggerTime_ms * 0.001D, false), true);
        }

        private void ReadBodyMotionFromFile(BodyMotionPointData current)
        {
            if (current == null) return;

            Sequencer.ExtraData extraData = null;

            switch (current.radius_mm as string)
            {
                case "TURN_IN_PLACE":
                    Sequencer.ExtraTurnInPlaceData turnInPlaceData = new Sequencer.ExtraTurnInPlaceData();
                    turnInPlaceData.Angle_deg = (int)((current.durationTime_ms * 0.001D) * current.speed);
                    extraData = turnInPlaceData;
                    break;
                case "STRAIGHT":
                    Sequencer.ExtraStraightData straightData = new Sequencer.ExtraStraightData();
                    straightData.Speed_mms = (int)current.speed;
                    extraData = straightData;
                    break;
                default: // Arc
                    Sequencer.ExtraArcData arcData = new Sequencer.ExtraArcData();
                    arcData.Speed_mms = (int)current.speed;
                    arcData.Radius_mm = Convert.ToInt32(current.radius_mm);
                    extraData = arcData;
                    break;
            }

            if (extraData.Length < MoveSelectedDataPoints.DELTA_X)
            {
                return;
            }

            extraData.Length = current.durationTime_ms * 0.001D;
            ActionManager.Do(new Sequencer.EnableChart(bodyMotion), true);
            ActionManager.Do(new Sequencer.AddDataPoint(bodyMotion.chart, extraData, current.triggerTime_ms * 0.001D, false), true);
        }

        private void ReadHeadAngleFromFile(HeadAnglePointData current, HeadAnglePointData next)
        {
            if (current == null) return;

            ActionManager.Do(new EnableChart(headAngle), true);

            if (next == null) // if last point, move to correct y
            {
                DataPoint last = headAngle.chart.Series[0].Points[headAngle.chart.Series[0].Points.Count - 1];
                double x = headAngle.chart.ChartAreas[0].AxisX.Maximum;
                ActionManager.Do(new MoveDataPoint(last, x, current.angle_deg), true);
            }
            else
            {
                ActionManager.Do(new AddDataPoint(headAngle.chart, next.triggerTime_ms * 0.001D, current.angle_deg, false), true);
            }
        }

        private void ReadChartsFromFile(string text)
        {
            if (text == null || chartForms == null) return;

            Dictionary<string, PointData[]> animation = null;

            try
            {
                animation = JsonConvert.DeserializeObject<Dictionary<string, PointData[]>>(text,
                    new JsonSerializerSettings { TypeNameHandling = TypeNameHandling.Auto });
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
                return;
            }

            ActionManager.Do(new ChangeAllChartDurations(chartForms, GetDurationNeeded(animation)));

            DisableAllCharts(); // disable all charts, only enable them if the file has points for them

            foreach (string key in animation.Keys)
            {
                PointData[] points = animation[key];

                for (int i = 0; i < points.Length; ++i)
                {
                    switch (points[i].Name)
                    {
                        case HeadAnglePointData.NAME:
                            ReadHeadAngleFromFile(points[i] as HeadAnglePointData, i+1 < points.Length ? points[i+1] as HeadAnglePointData : null);
                            break;
                        case LiftHeightPointData.NAME:
                            ReadLiftHeightFromFile(points[i] as LiftHeightPointData, i+1 < points.Length ? points[i+1] as LiftHeightPointData : null);
                            break;
                        case AudioRobotPointData.NAME:
                            ReadAudioRobotFromFile(points[i] as AudioRobotPointData);
                            break;
                        case AudioDevicePointData.NAME:
                            ReadAudioDeviceFromFile(points[i] as AudioDevicePointData);
                            break;
                        case FaceAnimationPointData.NAME:
                            ReadFaceAnimationImageFromFile(points[i] as FaceAnimationPointData);
                            break;
                        case ProceduralFacePointData.NAME:
                            ReadProceduralFaceFromFile(points[i] as ProceduralFacePointData);
                            break;
                        case BodyMotionPointData.NAME:
                            ReadBodyMotionFromFile(points[i] as BodyMotionPointData);
                            break;
                    }
                }
            }

            ActionManager.Do(new UnselectAllCharts(chartForms), true);
        }

        private double GetDurationNeeded(Dictionary<string, PointData[]> animation)
        {
            double x = -1;

            foreach (string key in animation.Keys)
            {
                PointData[] points = animation[key];

                for (int i = 0; i < points.Length; ++i)
                {
                    double newX = GetTimeNeededFromFile(points[i]);

                    if (newX > x)
                    {
                        x = newX;
                    }
                }
            }

            return x;
        }

        private void DisableAllCharts()
        {
            foreach (ChartForm chartForm in chartForms)
            {
                if (chartForm.Enabled)
                {
                    ActionManager.Do(new DisableChart(chartForm));
                }
            }
        }
    }
}
