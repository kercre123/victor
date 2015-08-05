using System.Linq;
using System.Windows.Forms.DataVisualization.Charting;
using System.Collections.Generic;
using System.Drawing;

namespace AnimationTool
{
    public class RemoveDataPoint : AddDataPoint
    {
        public RemoveDataPoint(DataPoint dataPoint, Chart chart)
        {
            this.dataPoint = dataPoint;
            this.chart = chart;
            points = chart.Series[0].Points;
        }

        public override bool Do()
        {
            if (dataPoint == null || points == null || dataPoint.IsEmpty || dataPoint.Color == Color.Red) return false;

            points.Remove(dataPoint);

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            points.Add(dataPoint);

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
        }
    }
}
