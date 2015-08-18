using System.Linq;
using System.Windows.Forms.DataVisualization.Charting;
using System.Collections.Generic;
using System.Drawing;

namespace AnimationTool.Sequencer
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
            if (dataPoint == null || points == null || dataPoint.IsEmpty || dataPoint.Color == Color.Red || dataPoint == points[0]) return false;

            points.Remove(dataPoint);

            chart.Refresh();

            return true;
        }

        public override void Undo()
        {
            points.Add(dataPoint);

            if (points.Count > 1)
            {
                DataPoint previewBar = null;

                for (int i = 0; i < points.Count; ++i)
                {
                    if (points[i].Color == Color.Red)
                    {
                        previewBar = points[i];
                        points.RemoveAt(i);
                        break;
                    }
                }

                List<DataPoint> inOrder = points.OrderBy(x => x.YValues[0]).ToList();

                if (previewBar != null)
                {
                    inOrder.Insert(inOrder.Count-1, previewBar);
                }

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
