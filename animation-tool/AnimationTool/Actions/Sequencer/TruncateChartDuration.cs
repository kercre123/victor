using System;
using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool.Sequencer
{
    public class TruncateChartDuration : AnimationTool.TruncateChartDuration
    {
        protected new MoveDataPoint moveDataPoint;
        protected new List<RemoveDataPoint> removeDataPoints;

        public TruncateChartDuration(Chart chart, double xValueNew)
        {
            if (chart == null) return;

            removeDataPoints = new List<RemoveDataPoint>();

            this.chart = chart;
            this.xValueNew = xValueNew;

            xValueOld = chart.ChartAreas[0].AxisY.Maximum;
            points = chart.Series[0].Points;
        }

        public override bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null || points.Count < 2) return false;

            chart.ChartAreas[0].AxisY.Maximum = xValueNew;
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            if (xValueOld > xValueNew) // if time decreased, remove unwanted points
            {
                for (int i = 1; i < points.Count - 1; ++i) // ignore first and last point
                {
                    DataPoint dataPoint = points[i];

                    if (dataPoint.YValues[1] > xValueNew)
                    {
                        removeDataPoints.Add(new RemoveDataPoint(dataPoint, chart));
                    }
                }

                foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
                {
                    removeDataPoint.Do();
                }
            }

            moveDataPoint = new MoveDataPoint(points[points.Count - 1], xValueNew); // move last to end
            moveDataPoint.Do();

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            chart.ChartAreas[0].AxisY.Maximum = xValueOld;
            chart.ChartAreas[0].AxisY.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);

            moveDataPoint.Undo();

            foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
            {
                removeDataPoint.Undo();
            }

            chart.Refresh();
        }
    }
}
