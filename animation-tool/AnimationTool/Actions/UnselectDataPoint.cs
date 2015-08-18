using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool
{
    public class UnselectDataPoint : Action
    {
        public static readonly Color MarkerColor = Color.LightGreen;
        public static readonly Color BorderColor = Color.Gray;
        public const int MarkerSize = 8;
        public const int BorderWidth = 2;

        protected DataPoint dataPoint;

        public UnselectDataPoint(DataPoint dataPoint)
        {
            this.dataPoint = dataPoint;
        }

        public virtual bool Do()
        {
            if (dataPoint == null || dataPoint.MarkerSize == MarkerSize || dataPoint.BorderColor == BorderColor) return false;

            dataPoint.MarkerColor = MarkerColor;
            dataPoint.MarkerSize = MarkerSize;
            dataPoint.BorderColor = BorderColor;
            dataPoint.BorderWidth = BorderWidth;

            return true;
        }

        public virtual void Undo()
        {
            if (dataPoint == null) return;

            dataPoint.MarkerColor = SelectDataPoint.MarkerColor;
            dataPoint.MarkerSize = SelectDataPoint.MarkerSize;
            dataPoint.BorderColor = SelectDataPoint.BorderColor;
            dataPoint.BorderWidth = SelectDataPoint.BorderWidth;
        }
    }
}
