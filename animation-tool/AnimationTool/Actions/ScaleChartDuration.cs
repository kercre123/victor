using System;
using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class ScaleChartDuration : ChangeChartDuration
    {
        protected List<MoveDataPoint> moveDataPoints;
        protected List<RemoveDataPoint> removeDataPoints;

        protected ScaleChartDuration() { }

        public ScaleChartDuration(Chart chart, double xValueNew)
        {
            removeDataPoints = new List<RemoveDataPoint>();
            moveDataPoints = new List<MoveDataPoint>();

            this.chart = chart;
            points = chart.Series[0].Points;
            xValueOld = chart.ChartAreas[0].AxisX.Maximum;
            this.xValueNew = xValueNew;
        }

        public override bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null) return false;
            
            chart.ChartAreas[0].AxisX.Maximum = xValueNew;
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            for (int i = 0; i < points.Count; ++i)
            {
                DataPoint dataPoint = points[i];

                double newX = Math.Round(xValueNew * (dataPoint.XValue / xValueOld), 1);
                moveDataPoints.Add(new MoveDataPoint(dataPoint, newX, dataPoint.YValues[0]));
                moveDataPoints[i].Do();
            }

            chart.Refresh();

            for (int i = 1; i < points.Count; ++i) // remove overlapping data points and average out their y values
            {
                DataPoint a = chart.Series[0].Points[i-1];
                DataPoint b = chart.Series[0].Points[i];

                if (a.IsEmpty || b.IsEmpty) continue;

                if (Math.Round(a.XValue, 1) == Math.Round(b.XValue, 1))
                {
                    removeDataPoints.Add(new RemoveDataPoint(b, chart));
                    if (Math.Round(a.XValue, 1) > 0)
                    {
                        moveDataPoints.Add(new MoveDataPoint(a, a.XValue, (a.XValue + b.XValue) * 0.5));
                        moveDataPoints[moveDataPoints.Count - 1].Do();
                    }
                }
            }

            foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
            {
                removeDataPoint.Do();
            }

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            if (chart == null) return;

            chart.ChartAreas[0].AxisX.Maximum = Math.Round(xValueOld, 1);
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);

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
