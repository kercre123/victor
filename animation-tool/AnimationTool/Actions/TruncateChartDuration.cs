using System;
using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class TruncateChartDuration : ChangeChartDuration
    {
        protected AddDataPoint addDataPoint;
        protected MoveDataPoint moveDataPoint;
        protected List<RemoveDataPoint> removeDataPoints;

        protected TruncateChartDuration() { }

        public TruncateChartDuration(Chart chart, double xValueNew)
        {
            removeDataPoints = new List<RemoveDataPoint>();

            this.chart = chart;
            points = chart.Series[0].Points;
            this.xValueNew = Math.Round(xValueNew, 1);
            xValueOld = Math.Round(chart.ChartAreas[0].AxisX.Maximum, 1);
        }

        public override bool Do()
        {
            if (chart == null || xValueOld == xValueNew || points == null || points.Count < 2) return false;

            DataPoint beforeLast = points[points.Count - 2];
            DataPoint last = new DataPoint(points[points.Count - 1].XValue, points[points.Count - 1].YValues[0]); // make copy of last

            chart.ChartAreas[0].AxisX.Maximum = Math.Round(xValueNew, 1);
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueNew * 0.1, 1), 0.1, 0.5);

            if (xValueOld > xValueNew) // if time decreased, remove unwanted points
            {
                for (int i = 1; i < points.Count - 1; ++i) // ignore first and last point
                {
                    DataPoint dataPoint = points[i];

                    if (dataPoint.XValue >= xValueNew) // if x value equals new length, set as new last data point value
                    {
                        if (Math.Abs(xValueNew - dataPoint.XValue) < Math.Abs(xValueNew - last.XValue))
                        {
                            last.YValues[0] = dataPoint.YValues[0];
                            last.XValue = dataPoint.XValue;
                            beforeLast = points[i - 1];
                        }

                        removeDataPoints.Add(new RemoveDataPoint(dataPoint, chart));
                    }
                }

                foreach (RemoveDataPoint removeDataPoint in removeDataPoints)
                {
                    removeDataPoint.Do();
                }

                double slope = (last.YValues[0] - beforeLast.YValues[0]) / (last.XValue - beforeLast.XValue);
                double y = slope * (xValueNew - beforeLast.XValue) + beforeLast.YValues[0];
                last = points[points.Count - 1];

                moveDataPoint = new MoveDataPoint(last, xValueNew, y); // move last to end
                moveDataPoint.Do();
            }
            else
            {
                if (points.Count > 2) //if time increased, add point with same value as previous last
                {
                    addDataPoint = new AddDataPoint(chart, xValueNew, last.YValues[0]);
                    addDataPoint.Do();
                }
                else // if no points have been added, move last to end
                {
                    last = points[points.Count - 1];

                    moveDataPoint = new MoveDataPoint(last, xValueNew, last.YValues[0]);
                    moveDataPoint.Do();
                }
            }

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            if (chart == null) return;

            chart.ChartAreas[0].AxisX.Maximum = Math.Round(xValueOld, 1);
            chart.ChartAreas[0].AxisX.LabelStyle.Interval = Clamp(Math.Round(xValueOld * 0.1, 1), 0.1, 0.5);

            if (addDataPoint != null)
            {
                addDataPoint.Undo();
            }

            if(moveDataPoint != null)
            {
                moveDataPoint.Undo();
            }

            foreach(RemoveDataPoint removeDataPoint in removeDataPoints)
            {
                removeDataPoint.Undo();
            }

            chart.Refresh();
        }
    }
}
