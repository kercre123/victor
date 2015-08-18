using System;
using System.Linq;
using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class AddDataPoint : Action
    {
        public DataPoint dataPoint { get; protected set; }
        protected Chart chart;
        protected DataPointCollection points;

        protected SelectDataPoint selectDataPoint;

        protected AddDataPoint() { }

        public AddDataPoint(Chart chart, double xValue, double yValue, bool select = true, bool unselectAllDataPoints = true)
        {
            if (chart == null) return;

            dataPoint = new DataPoint();

            dataPoint.XValue = Math.Round(xValue, 1);
            dataPoint.YValues[0] = Math.Round(yValue, 0);

            dataPoint.ToolTip = dataPoint.XValue + ", " + dataPoint.YValues[0];

            dataPoint.MarkerColor = UnselectDataPoint.MarkerColor;
            dataPoint.MarkerSize = UnselectDataPoint.MarkerSize;
            dataPoint.BorderColor = UnselectDataPoint.BorderColor;
            dataPoint.BorderWidth = UnselectDataPoint.BorderWidth;

            points = chart.Series[0].Points;

            if (select)
            {
                selectDataPoint = new SelectDataPoint(dataPoint, unselectAllDataPoints ? chart : null);
            }

            this.chart = chart;
        }

        public virtual bool Do()
        {
            if (dataPoint == null || chart == null || points.Contains(dataPoint)) return false;

            points.Add(dataPoint);

            if (selectDataPoint != null)
            {
                selectDataPoint.Do();
            }

            if (points.Count > 1)
            {
                List<DataPoint> inOrder = points.OrderBy(x => x.XValue).ToList();

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

        public virtual void Undo()
        {
            selectDataPoint.Undo();

            points.Remove(dataPoint);

            chart.Refresh();
        }

        protected virtual bool IsSame(List<DataPoint> inOrder)
        {
            bool same = true;

            for (int i = 0; i < inOrder.Count; ++i)
            {
                if (inOrder[i].XValue != points[i].XValue)
                {
                    same = false;
                    break;
                }
            }

            return same;
        }
    }
}
