using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class MoveDataPoint : Action
    {
        public DataPoint dataPoint { get; protected set; }

        protected double xValue;
        protected double yValue;
        protected double previousX;
        protected double previousY;

        protected MoveDataPoint() { }

        public MoveDataPoint(DataPoint dataPoint, double xValue, double yValue)
        {
            this.dataPoint = dataPoint;
            this.xValue = xValue;
            this.yValue = yValue;

            previousX = dataPoint.XValue;
            previousY = dataPoint.YValues[0];
        }

        public virtual bool Do()
        {
            if (dataPoint == null) return false;

            dataPoint.XValue = Math.Round(xValue, 1);
            dataPoint.YValues[0] = Math.Round(yValue, 0);
            dataPoint.ToolTip = dataPoint.XValue + ", " + dataPoint.YValues[0];

            return true;
        }

        public virtual void Undo()
        {
            dataPoint.XValue = Math.Round(previousX, 1);
            dataPoint.YValues[0] = Math.Round(previousY, 0);
            dataPoint.ToolTip = dataPoint.XValue + ", " + dataPoint.YValues[0];
        }
    }
}
