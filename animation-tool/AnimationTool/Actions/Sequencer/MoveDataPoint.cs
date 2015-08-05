using System.Windows.Forms.DataVisualization.Charting;
using System;
using System.IO;

namespace AnimationTool.Sequencer
{
    public class MoveDataPoint : AnimationTool.MoveDataPoint
    {
        protected ExtraData extraData;

        protected MoveDataPoint() { }

        public MoveDataPoint(DataPoint dataPoint, double yValue)
        {
            this.dataPoint = dataPoint;
            this.yValue = yValue;

            previousY = dataPoint.YValues[0];

            extraData = ExtraData.Entries[dataPoint.GetCustomProperty(ExtraData.Key)];
        }

        public override bool Do()
        {
            if (dataPoint == null) return false;

            dataPoint.SetValueY(Math.Round(yValue, 1), Math.Round(yValue + extraData.Length, 1));
            dataPoint.ToolTip = extraData.FileName + " (" + dataPoint.YValues[0] + ", " + dataPoint.YValues[1] + ")";

            if(!string.IsNullOrEmpty(extraData.Image) && File.Exists(extraData.Image))
            {
                dataPoint.BackImage = extraData.Image;
            }

            return true;
        }

        public override void Undo()
        {
            dataPoint.SetValueY(Math.Round(previousY, 1), Math.Round(previousY + extraData.Length, 1));
            dataPoint.ToolTip = extraData.FileName + " (" + dataPoint.YValues[0] + ", " + dataPoint.YValues[1] + ")";
        }
    }
}
