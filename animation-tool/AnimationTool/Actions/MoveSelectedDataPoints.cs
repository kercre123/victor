using System;
using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class MoveSelectedDataPoints : Action
    {
        public const double DELTA_Y = 1;
        public const double DELTA_X = 0.1;

        protected ChartArea chartArea;
        protected DataPointCollection points;
        protected Chart chart;

        protected List<DataPoint> selectedDataPoints;

        protected double nextLeft;
        protected double nextRight;

        protected List<MoveDataPoint> moveDataPoints;
        protected bool withKeyboard;

        public bool left { get; protected set; }
        public bool right { get; protected set; }
        public bool up { get; private set; }
        public bool down { get; private set; }

        protected MoveSelectedDataPoints() { }

        public MoveSelectedDataPoints(Chart chart, bool left, bool right, bool up, bool down)
        {
            if (chart == null) return;

            selectedDataPoints = new List<DataPoint>();
            points = chart.Series[0].Points;
            chartArea = chart.ChartAreas[0];
            this.chart = chart;
            moveDataPoints = new List<MoveDataPoint>();
            this.left = left;
            this.right = right;
            this.up = up;
            this.down = down;
        }

        public bool Do()
        {
            if(!Try())
            {
                return false;
            }

            return UpdateValues(left, right, up, down);
        }

        public void Undo()
        {
            foreach (MoveDataPoint action in moveDataPoints)
            {
                action.Undo();
            }

            chart.Refresh();
        }

        protected virtual void GetSelectedDataPoints()
        {
            if (selectedDataPoints == null || points == null || selectedDataPoints.Count > 0) return;

            nextLeft = -1;
            nextRight = -1;

            for(int i = 1; i < points.Count; ++i)
            {
                DataPoint dp = points[i];

                if (dp.MarkerSize == SelectDataPoint.MarkerSize)
                {
                    selectedDataPoints.Add(dp);

                    if (points[i-1].MarkerSize != SelectDataPoint.MarkerSize) // find last
                    {
                        nextLeft = points[i-1].XValue;
                    }

                    if (i+1 < points.Count && nextRight < 0 && points[i+1].MarkerSize != SelectDataPoint.MarkerSize) // find first
                    {
                        nextRight = points[i+1].XValue;
                    }
                }
            }
        }

        protected void Up()
        {
            GetSelectedDataPoints();

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round(chartArea.AxisY.Maximum - dp.YValues[0], 0);
                if (distance < DELTA_Y)
                {
                    up = false;
                    return;
                }
            }
        }

        protected void Down()
        {
            GetSelectedDataPoints();

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round(dp.YValues[0] - chartArea.AxisY.Minimum, 0);
                if (distance < DELTA_Y)
                {
                    down = false;
                    return;
                }
            }
        }

        protected virtual void Right()
        {
            GetSelectedDataPoints();

            if (nextRight < 0 || nextLeft < 0) //don't allow x movement of first and last point
            {
                right = false;
                return;
            }

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round((nextRight - DELTA_X) - dp.XValue, 1);
                if (distance < DELTA_X) //stop value from going equal to the next data point
                {
                    right = false;
                    return;
                }
            }
        }

        protected virtual void Left()
        {
            GetSelectedDataPoints();

            if (nextRight < 0 || nextLeft < 0) //don't allow x movement of first and last point
            {
                left = false;
                return;
            }

            foreach (DataPoint dp in selectedDataPoints)
            {
                double distance = Math.Round(dp.XValue - (nextLeft + DELTA_X), 1);
                if (distance < DELTA_X) //stop value from going equal to the next data point
                {
                    left = false;
                    return;
                }
            }
        }

        public virtual bool UpdateValues(bool left, bool right, bool up, bool down)
        {
            double changeX = left ? -DELTA_X : right ? DELTA_X : 0.0;
            double changeY = down ? -DELTA_Y : up ? DELTA_Y : 0.0;

            for (int i = 0; i < selectedDataPoints.Count; ++i)
            {
                DataPoint dp = selectedDataPoints[i];
                MoveDataPoint action = new MoveDataPoint(dp, dp.XValue + changeX, dp.YValues[0] + changeY);

                if (!action.Do())
                {
                    Undo();
                    return false;
                }

                moveDataPoints.Add(action);
            }

            return true;
        }

        public virtual bool Try()
        {
            if (points == null || chartArea == null || selectedDataPoints == null) return false;

            if (up)
            {
                Up();
            }
            else if (down)
            {
                Down();
            }

            if (left)
            {
                Left();
            }
            else if (right)
            {
                Right();
            }

            return left || right || up || down;
        }
    }
}
