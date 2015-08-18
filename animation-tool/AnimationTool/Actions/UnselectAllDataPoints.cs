using System.Collections.Generic;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class UnselectAllDataPoints : Action
    {
        protected List<UnselectDataPoint> unselectDataPoints;
        protected DataPointCollection points;

        public UnselectAllDataPoints(Chart chart)
        {
            if (chart == null) return;

            unselectDataPoints = new List<UnselectDataPoint>();
            points = chart.Series[0].Points;
        }

        public virtual bool Do()
        {
            if (unselectDataPoints == null) return false;

            for (int i = 0; i < points.Count; ++i)
            {
                UnselectDataPoint unselectDataPoint = new UnselectDataPoint(points[i]);

                if (unselectDataPoint.Do())
                {
                    unselectDataPoints.Add(unselectDataPoint);
                }
            }

            return true;
        }

        public virtual void Undo()
        {
            if (unselectDataPoints == null) return;

            foreach (UnselectDataPoint action in unselectDataPoints)
            {
                action.Undo();
            }
        }
    }
}

