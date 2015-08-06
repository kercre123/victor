using System;
using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool.Sequencer
{
    public class MoveSelectedDataPoints : AnimationTool.MoveSelectedDataPoints
    {
        protected new List<MoveDataPoint> moveDataPoints;

        protected MoveSelectedDataPoints() { }

        public MoveSelectedDataPoints(Chart chart, bool left, bool right)
        {
            if (chart == null) return;

            selectedDataPoints = new List<DataPoint>();
            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];
            this.chart = chart;
            moveDataPoints = new List<MoveDataPoint>();
            this.left = left;
            this.right = right;
        }

        public override bool Try()
        {
            if (points == null || chartArea == null || selectedDataPoints == null) return false;

            if (left)
            {
                Left();
            }
            else if (right)
            {
                Right();
            }

            return left || right;
        }

        protected override void Right()
        {
            GetSelectedDataPoints();

            if (nextRight < 0 || nextLeft < 0) //don't allow x movement of first and last point
            {
                right = false;
                return;
            }

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round(nextRight - dp.YValues[1], 1);
                if (distance < DELTA_X) //stop value from going equal to the next data point
                {
                    right = false;
                    return;
                }
            }
        }

        protected override void Left()
        {
            GetSelectedDataPoints();

            if (nextRight < 0 || nextLeft < 0) //don't allow x movement of first and last point
            {
                left = false;
                return;
            }

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round(dp.YValues[0] - nextLeft, 1);
                if (distance < DELTA_X) //stop value from going equal to the next data point
                {
                    left = false;
                    return;
                }
            }
        }

        protected override void GetSelectedDataPoints()
        {
            if (selectedDataPoints == null || points == null || selectedDataPoints.Count > 0) return;

            nextLeft = -1;
            nextRight = -1;

            List<DataPoint> dataPoints = new List<DataPoint>();
            dataPoints.AddRange(points);

            for (int i = 0; i < dataPoints.Count; ++i)
            {
                DataPoint dp = dataPoints[i];

                if (dp.Color == Color.Red)
                {
                    selectedDataPoints.Add(dp);
                    dataPoints.RemoveAt(i);
                    break;
                }
            }

            for (int i = 1; i < dataPoints.Count; ++i)
            {
                DataPoint dp = dataPoints[i];

                if (dp.MarkerColor == SelectDataPoint.MarkerColor)
                {
                    selectedDataPoints.Add(dp);

                    if (dataPoints[i-1].MarkerColor != SelectDataPoint.MarkerColor) // find last
                    {
                        nextLeft = dataPoints[i-1].YValues[1];
                    }

                    if (i+1 < dataPoints.Count && nextRight < 0 && dataPoints[i+1].MarkerColor != SelectDataPoint.MarkerColor) // find first
                    {
                        nextRight = dataPoints[i+1].YValues[0];
                    }
                }
            }
        }

        public override bool UpdateValues(bool left, bool right, bool up, bool down)
        {
            if (!left && !right) return false;

            double changeX = left ? -DELTA_X : DELTA_X;

            for (int i = 0; i < selectedDataPoints.Count; ++i)
            {
                DataPoint dp = selectedDataPoints[i];
                MoveDataPoint action = null;

                if (dp.Color != Color.Red)
                {
                    action = new MoveDataPoint(dp, dp.YValues[0] + changeX);
                }
                else
                {
                    action = new FaceAnimation.MovePreviewBar(dp, dp.YValues[0] + changeX);
                }

                if (!action.Do())
                {
                    Undo();
                    return false;
                }

                moveDataPoints.Add(action);
            }

            return true;
        }
    }
}
