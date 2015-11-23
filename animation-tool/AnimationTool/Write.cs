using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System.IO;
using Newtonsoft.Json;
using System.Drawing;
using System;

namespace AnimationTool
{
    partial class MainForm
    {
        private string WriteToFile()
        {
            Dictionary<string, PointData[]> animation = new Dictionary<string, PointData[]>();

            string name = Path.GetFileNameWithoutExtension(currentFile);

            List<PointData> points = new List<PointData>();

            foreach (ChartForm chartForm in chartForms)
            {
                if (chartForm.chart.Enabled)
                {
                    switch (chartForm.chart.Name)
                    {
                        case "cHeadAngle":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count - 1; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];
                                DataPoint next = chartForm.chart.Series[0].Points[i + 1];

                                points.Add(WriteHeadAngleToFile(current, next));
                            }
                            break;
                        case "cLiftHeight":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count - 1; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];
                                DataPoint next = chartForm.chart.Series[0].Points[i + 1];

                                points.Add(WriteLiftHeightToFile(current, next));
                            }
                            break;
                        case "cAudioRobot":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];

                                if (current.IsEmpty) continue;

                                points.Add(WriteAudioRobotToFile(current));
                            }
                            break;
                        case "cAudioDevice":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];

                                if (current.IsEmpty) continue;

                                points.Add(WriteAudioDeviceToFile(current));
                            }
                            break;
                        case "cFaceAnimation":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];

                                if (current.IsEmpty || current.Color == Color.Red) continue;

                                points.Add(WriteFaceAnimationToFile(current));
                            }
                            break;
                        case "cProceduralFace":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];

                                if (current.IsEmpty) continue;

                                points.Add(WriteProceduralFaceToFile(current));
                            }
                            break;
                        case "cBodyMotion":
                            for (int i = 0; i < chartForm.chart.Series[0].Points.Count; ++i)
                            {
                                DataPoint current = chartForm.chart.Series[0].Points[i];

                                if (current.IsEmpty) continue;

                                points.Add(WriteBodyMotionToFile(current));
                            }
                            break;
                    }
                }
            }

            animation.Add(name, points.ToArray());

            string json = null;

            try
            {
                json = JsonConvert.SerializeObject(animation, new JsonSerializerSettings
                {
                    TypeNameHandling = TypeNameHandling.Auto,
                    Formatting = Formatting.Indented
                });
            }
            catch (Exception e)
            {
                MessageBox.Show(e.Message);
            }

            return json;
        }

        private HeadAnglePointData WriteHeadAngleToFile(DataPoint current, DataPoint next)
        {
            HeadAnglePointData pointData = new HeadAnglePointData();
            pointData.triggerTime_ms = (int)(current.XValue * 1000); // convert to ms
            pointData.durationTime_ms = (int)((next.XValue - current.XValue) * 1000); // convert to ms
            pointData.angle_deg = (int)next.YValues[0];
            pointData.angleVariability_deg = 0;

            return pointData;
        }

        private LiftHeightPointData WriteLiftHeightToFile(DataPoint current, DataPoint next)
        {
            LiftHeightPointData pointData = new LiftHeightPointData();
            pointData.triggerTime_ms = (int)(current.XValue * 1000); // convert to ms
            pointData.durationTime_ms = (int)((next.XValue - current.XValue) * 1000); // convert to ms
            pointData.height_mm = (int)next.YValues[0];
            pointData.heightVariability_mm = 0;

            return pointData;
        }

        private AudioRobotPointData WriteAudioRobotToFile(DataPoint current)
        {
            AudioRobotPointData pointData = new AudioRobotPointData();

            return WriteAudioToFile(pointData, current);
        }

        private AudioDevicePointData WriteAudioDeviceToFile(DataPoint current)
        {
            AudioDevicePointData pointData = new AudioDevicePointData();

            return (AudioDevicePointData)WriteAudioToFile(pointData, current);
        }

        private AudioRobotPointData WriteAudioToFile(AudioRobotPointData pointData, DataPoint current)
        {
            string key = current.GetCustomProperty(Sequencer.ExtraData.Key);
            Sequencer.ExtraAudioData extraData = Sequencer.ExtraData.Entries[key] as Sequencer.ExtraAudioData;
            pointData.triggerTime_ms = (int)(current.YValues[0] * 1000); // convert to ms
            pointData.durationTime_ms = (int)(extraData.Length * 1000); // convert to ms
            pointData.volume = Math.Round(extraData.Volume, 1);

            pointData.audioName = new List<string>();

            for (int i = 0; i < extraData.Sounds.Count; ++i)
            {
                if (extraData.Sounds[i].StartsWith(Sequencer.ExtraAudioData.FullPath))
                {
                    string audioName = extraData.Sounds[i].Remove(0, Sequencer.ExtraAudioData.FullPath.Length);

                    if (audioName.StartsWith("\\"))
                    {
                        audioName = audioName.Remove(0, 1);
                    }

                    audioName = audioName.Replace("\\", @"/");
                    pointData.audioName.Add(audioName);
                }
                else
                {
                    MessageBox.Show(extraData.Sounds[i] + " is not in root directory " + Sequencer.ExtraAudioData.FullPath);
                }
            }

            if (extraData.FileNameWithPath.StartsWith(Sequencer.ExtraAudioData.FullPath))
            {
                pointData.pathFromRoot = extraData.FileNameWithPath.Remove(0, Sequencer.ExtraAudioData.FullPath.Length);
            }
            else
            {
                MessageBox.Show(extraData.FileNameWithPath + " is not in root directory " + Sequencer.ExtraAudioData.FullPath);
            }

            return pointData;
        }

        private FaceAnimationPointData WriteFaceAnimationToFile(DataPoint current)
        {
            FaceAnimationPointData pointData = new FaceAnimationPointData();
            string key = current.GetCustomProperty(Sequencer.ExtraData.Key);
            Sequencer.ExtraFaceAnimationData extraData = Sequencer.ExtraData.Entries[key] as Sequencer.ExtraFaceAnimationData;
            pointData.triggerTime_ms = (int)(current.YValues[0] * 1000); // convert to ms
            pointData.durationTime_ms = (int)(extraData.Length * 1000); // convert to ms

            if (extraData.FileNameWithPath.StartsWith(Sequencer.ExtraFaceAnimationData.FullPath))
            {
                pointData.pathFromRoot = extraData.FileNameWithPath.Remove(0, Sequencer.ExtraFaceAnimationData.FullPath.Length);

                if (pointData.pathFromRoot.StartsWith("\\"))
                {
                    pointData.animName = pointData.pathFromRoot.Remove(0, 1);
                }
            }
            else
            {
                MessageBox.Show(extraData.FileNameWithPath + " is not in root directory " + Sequencer.ExtraFaceAnimationData.FullPath);
            }

            return pointData;
        }

        private ProceduralFacePointData WriteProceduralFaceToFile(DataPoint current)
        {
            string key = current.GetCustomProperty(Sequencer.ExtraData.Key);
            Sequencer.ExtraProceduralFaceData extraData = Sequencer.ExtraData.Entries[key] as Sequencer.ExtraProceduralFaceData;
            ProceduralFacePointData pointData = extraData.GetProceduralFacePointData();

            pointData.triggerTime_ms = (int)(current.YValues[0] * 1000); // convert to ms
            pointData.durationTime_ms = 0;

            return pointData;
        }

        private BodyMotionPointData WriteBodyMotionToFile(DataPoint current)
        {
            BodyMotionPointData pointData = new BodyMotionPointData();
            Sequencer.ExtraData extraData = Sequencer.ExtraData.Entries[current.GetCustomProperty(Sequencer.ExtraData.Key)];

            pointData.triggerTime_ms = (int)(current.YValues[0] * 1000); // convert to ms
            pointData.durationTime_ms = (int)(extraData.Length * 1000); // convert to ms

            Sequencer.ExtraStraightData straightData = extraData as Sequencer.ExtraStraightData;

            if (straightData != null)
            {
                pointData.radius_mm = "STRAIGHT";
                pointData.speed = straightData.Speed_mms;
            }
            else
            {
                Sequencer.ExtraArcData arcData = extraData as Sequencer.ExtraArcData;

                if (arcData != null)
                {
                    pointData.radius_mm = arcData.Radius_mm;
                    pointData.speed = arcData.Speed_mms;
                }
                else
                {
                    Sequencer.ExtraTurnInPlaceData turnInPlaceData = extraData as Sequencer.ExtraTurnInPlaceData;

                    if (turnInPlaceData != null)
                    {
                        pointData.radius_mm = "TURN_IN_PLACE";
                        pointData.speed = turnInPlaceData.Angle_deg / turnInPlaceData.Length; // convert to ms
                    }
                }
            }

            return pointData;
        }
    }
}
