using System.Windows.Forms.DataVisualization.Charting;
﻿using System.Windows.Forms;
using System;
using System.Collections.Generic;
using System.Linq;
using System.IO;
using System.Drawing;

namespace AnimationTool.Sequencer
{
    public class AddDataPoint : AnimationTool.AddDataPoint
    {
        protected ChartArea chartArea;
        protected ExtraData extraData;
        protected double start;

        public static EventHandler ChangeDuration;

        protected AddDataPoint() { }

        public AddDataPoint(Chart chart, ExtraData extraData, double start, PictureBox pictureBox, bool unselectAllDataPoints = true)
        {
            Initialize(chart, extraData, start, pictureBox, pictureBox != null, unselectAllDataPoints);
        }

        public AddDataPoint(Chart chart, ExtraData extraData, double start, bool select = true, bool unselectAllDataPoints = true)
        {
            Initialize(chart, extraData, start, null, select, unselectAllDataPoints);
        }

        protected void Initialize(Chart chart, ExtraData extraData, double start, PictureBox pictureBox, bool select, bool unselectAllDataPoints)
        {
            if (chart == null) return;

            this.chart = chart;
            this.start = start;
            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];

            dataPoint = new DataPoint();
            this.extraData = extraData;

            dataPoint.XValue = 0.5D;
            dataPoint.SetValueY(Math.Round(start, 1), Math.Round(start + extraData.Length, 1));

            if(!extraData.Exists)
            {
                dataPoint.Color = Color.LightGray;
            }

            if (!string.IsNullOrEmpty(extraData.Image) && File.Exists(extraData.Image))
            {
                dataPoint.BackImageWrapMode = ChartImageWrapMode.Scaled;
                dataPoint.BackImage = extraData.Image;
            }

            if (select)
            {
                if (pictureBox == null) // if no picture box
                {
                    selectDataPoint = new SelectDataPoint(dataPoint, unselectAllDataPoints ? chart : null);
                }
                else // else if picture box, add preview bar
                {
                    selectDataPoint = new FaceAnimation.SelectDataPoint(dataPoint, unselectAllDataPoints ? chart : null, pictureBox);
                }
            }

            ValidatePosition();
        }

        public override bool Do()
        {
            if (dataPoint == null || extraData == null || points.Contains(dataPoint)) return false;

            points.Add(dataPoint);
            dataPoint.SetCustomProperty(ExtraData.Key, "dp." + ++ExtraData.Count);
            ExtraData.Entries.Add(dataPoint.GetCustomProperty(ExtraData.Key), extraData);
            dataPoint.ToolTip = extraData.FileName + " (" + dataPoint.YValues[0] + ", " + dataPoint.YValues[1] + ")";

            if (selectDataPoint != null)
            {
                selectDataPoint.Do();
            }

            if (points.Count > 1)
            {
                DataPoint previewBar = null;

                for (int i = 0; i < points.Count; ++i)
                {
                    if (points[i].Color == Color.Red)
                    {
                        previewBar = points[i];
                        points.RemoveAt(i);
                        break;
                    }
                }

                List<DataPoint> inOrder = points.OrderBy(x => x.YValues[0]).ToList();

                if (previewBar != null)
                {
                    inOrder.Insert(inOrder.Count-1, previewBar);
                }

                if (!IsSame(inOrder))
                {
                    points.Clear();

                    foreach (DataPoint dp in inOrder)
                    {
                        points.Add(dp);
                    }
                }
            }

            chart.Refresh();

            return true;
        }

        protected void ValidatePosition()
        {
            if (dataPoint.YValues[1] > chartArea.AxisY.Maximum && dataPoint.YValues[0] < chartArea.AxisY.Maximum)
            {
                if (ChangeDuration != null)
                {
                    DialogResult result = MessageBox.Show("Do you want to change the duration?", "Cannot add data point of " + extraData.Length + " length here.", MessageBoxButtons.YesNo);

                    if (result == DialogResult.Yes) // scale
                    {
                        ChangeDuration(dataPoint.YValues[1], null);
                    }
                    else
                    {
                        dataPoint = null;
                    }
                }
                else
                {
                    dataPoint = null;
                }
            }
            else
            {
                for (int i = 0; i < points.Count; ++i)
                {
                    DataPoint dp = points[i];

                    if (dp == null || dp.IsEmpty) continue;

                    if (dataPoint.YValues[0] < dp.YValues[0] && dataPoint.YValues[1] > dp.YValues[0])
                    {
                        dataPoint = null;
                        break;
                    }
                }
            }

            if (dataPoint == null)
            {
                MessageBox.Show("Cannot add data point of " + extraData.Length + " length here.");
            }
        }

        public override void Undo()
        {
            points.Remove(dataPoint);
            ExtraData.Entries.Remove(dataPoint.Name);

            chart.Refresh();
        }

        protected override bool IsSame(List<DataPoint> inOrder)
        {
            bool same = true;

            for (int i = 0; i < inOrder.Count; ++i)
            {
                if (inOrder[i].YValues[0] != points[i].YValues[0])
                {
                    same = false;
                    break;
                }
            }

            return same;
        }
    }
}
