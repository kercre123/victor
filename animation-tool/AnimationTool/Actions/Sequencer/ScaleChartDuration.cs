using System;
using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool.Sequencer
{
    public class ScaleChartDuration : AnimationTool.ScaleChartDuration
    {
        protected new List<MoveDataPoint> moveDataPoints;
        protected new List<RemoveDataPoint> removeDataPoints;

        protected DataPoint previewBar;

        public ScaleChartDuration(Chart chart, double xValueNew)
        {
            if (chart == null) return;

            removeDataPoints = new List<RemoveDataPoint>();
            moveDataPoints = new List<MoveDataPoint>();

            this.chart = chart;
            points = chart.Series[0].Points;
            xValueOld = Math.Round(chart.ChartAreas[0].AxisY.Maximum, 1);
            this.xValueNew = Math.Round(xValueNew, 1);
        }

        public override bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null || points.Count < 2) return false;

            chart.ChartAreas[0].AxisY.Maximum = Math.Round(xValueNew, 1);
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            for (int i = 0; i < points.Count; ++i)
            {
                DataPoint dataPoint = points[i];

                double newX = xValueNew * (dataPoint.YValues[0] / xValueOld);
                moveDataPoints.Add(new MoveDataPoint(dataPoint, newX));
                moveDataPoints[i].Do();
            }

            for (int i = 0; i < points.Count; ++i)
            {
                if (points[i].Color == Color.Red)
                {
                    previewBar = points[i];
                    points.RemoveAt(i);
                    break;
                }
            }

            chart.Refresh();

            for (int i = 1; i < points.Count; ++i) // remove overlapping data points and tracks that go beyond the max
            {
                DataPoint a = points[i - 1];
                DataPoint b = points[i];

                if (a.IsEmpty || b.IsEmpty) continue;

                if ((a.YValues[0] > b.YValues[0] && a.YValues[0] < b.YValues[1]) || (a.YValues[1] > b.YValues[0] && a.YValues[1] < b.YValues[1]))
                {
                    removeDataPoints.Add(new RemoveDataPoint(b, chart));
                }
                else if (b.YValues[1] > xValueNew)
                {
                    removeDataPoints.Add(new RemoveDataPoint(b, chart));
                }
            }

            foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
            {
                removeDataPoint.Do();
            }

            if (previewBar != null)
            {
                points.Insert(points.Count - 1, previewBar);
            }

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            if (chart == null) return;

            chart.ChartAreas[0].AxisY.Maximum = Math.Round(xValueOld, 1);
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);

            foreach (MoveDataPoint moveDataPoint in moveDataPoints)
            {
                moveDataPoint.Undo();
            }

            foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
            {
                removeDataPoint.Undo();
            }

            chart.Refresh();
        }
    }
}
