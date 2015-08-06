using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class RemoveSelectedDataPoints : Action
    {
        protected DataPointCollection points;
        protected ChartArea chartArea;
        protected Chart chart;

        protected List<RemoveDataPoint> removeDataPoints;

        protected RemoveSelectedDataPoints() { }

        public RemoveSelectedDataPoints(Chart chart)
        {
            if (chart == null) return;

            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];
            removeDataPoints = new List<RemoveDataPoint>();
            this.chart = chart;
        }

        public virtual bool Do()
        {
            if (removeDataPoints == null || points == null) return false;

            for (int i = 0; i < points.Count; ++i)
            {
                DataPoint dataPoint = points[i];

                if (dataPoint.MarkerSize == SelectDataPoint.MarkerSize && 
                    dataPoint.XValue > chartArea.AxisX.Minimum && dataPoint.XValue < chartArea.AxisX.Maximum)
                {
                    removeDataPoints.Add(new RemoveDataPoint(dataPoint, chart));
                }
            }

            for (int i = 0; i < removeDataPoints.Count; ++i)
            {
                if(!removeDataPoints[i].Do())
                {
                    Undo();
                    return false;
                }
            }

            return true;
        }

        public virtual void Undo()
        {
            if (removeDataPoints == null) return;

            for (int i = 0; i < removeDataPoints.Count; ++i)
            {
                removeDataPoints[i].Undo();
            }
        }
    }
}
