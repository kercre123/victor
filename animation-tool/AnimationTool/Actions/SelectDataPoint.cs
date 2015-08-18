using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool
{
    public class SelectDataPoint : Action
    {
        public static readonly Color MarkerColor = Color.Orange;
        public static readonly Color BorderColor = Color.Orange;
        public const int MarkerSize = 10;
        public const int BorderWidth = 2;

        public DataPoint dataPoint { get; protected set; }
        protected UnselectAllDataPoints unselectAllDataPoints;

        protected SelectDataPoint() { }

        public SelectDataPoint(DataPoint dataPoint, Chart chart = null)
        {
            this.dataPoint = dataPoint;

            if (chart != null)
            {
                unselectAllDataPoints = new UnselectAllDataPoints(chart);
            }
        }

        public virtual bool Do()
        {
            if (dataPoint == null || dataPoint.MarkerSize == MarkerSize || dataPoint.BorderColor == BorderColor) return false;

            if (unselectAllDataPoints != null)
            {
                unselectAllDataPoints.Do();
            }

            dataPoint.MarkerColor = MarkerColor;
            dataPoint.MarkerSize = MarkerSize;
            dataPoint.BorderColor = BorderColor;
            dataPoint.BorderWidth = BorderWidth;

            return true;
        }

        public virtual void Undo()
        {
            if (dataPoint == null) return;

            if (unselectAllDataPoints != null)
            {
                unselectAllDataPoints.Undo();
            }

            dataPoint.MarkerColor = UnselectDataPoint.MarkerColor;
            dataPoint.MarkerSize = UnselectDataPoint.MarkerSize;
            dataPoint.BorderColor = UnselectDataPoint.BorderColor;
            dataPoint.BorderWidth = UnselectDataPoint.BorderWidth;
        }
    }
}
