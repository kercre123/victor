using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool.Sequencer
{
    public class RemoveSelectedDataPoints : AnimationTool.RemoveSelectedDataPoints
    {
        protected new List<RemoveDataPoint> removeDataPoints;

        public RemoveSelectedDataPoints(Chart chart)
        {
            if (chart == null) return;

            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];
            removeDataPoints = new List<RemoveDataPoint>();
            this.chart = chart;
        }

        public override bool Do()
        {
            if (removeDataPoints == null || points == null) return false;

            for (int i = 0; i < points.Count; ++i)
            {
                DataPoint dataPoint = points[i];

                if (dataPoint.MarkerSize == SelectDataPoint.MarkerSize && !dataPoint.IsEmpty)
                {
                    removeDataPoints.Add(new RemoveDataPoint(dataPoint, chart));
                }
            }

            for (int i = 0; i < removeDataPoints.Count; ++i)
            {
                if (!removeDataPoints[i].Do())
                {
                    removeDataPoints.RemoveAt(i--);
                }
            }

            return removeDataPoints.Count > 0;
        }

        public override void Undo()
        {
            if (removeDataPoints == null) return;

            for (int i = 0; i < removeDataPoints.Count; ++i)
            {
                removeDataPoints[i].Undo();
            }
        }
    }
}
