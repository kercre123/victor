using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class MoveSelectedDataPointsOfSelectedCharts : Action
    {
        protected bool left;
        protected bool right;
        protected bool up;
        protected bool down;

        protected List<MoveSelectedDataPoints> moveSelectedDataPoints;

        public MoveSelectedDataPointsOfSelectedCharts(List<Component> components, bool left, bool right, bool up, bool down)
        {
            this.left = left;
            this.right = right;
            this.up = up;
            this.down = down;

            Initialize(components);
        }

        private void Initialize(List<Component> components)
        {
            if (components == null) return;

            moveSelectedDataPoints = new List<MoveSelectedDataPoints>();

            foreach (Chart chart in components)
            {
                if (chart.BorderlineColor == SelectChart.borderlineColor)
                {
                    DataPoint dp = chart.Series[0].Points[0]; // if chart has sequencer point
                    bool sequencer = dp.IsCustomPropertySet(Sequencer.ExtraData.Key) &&
                        Sequencer.ExtraData.Entries.ContainsKey(dp.GetCustomProperty(Sequencer.ExtraData.Key));

                    if (sequencer)
                    {
                        moveSelectedDataPoints.Add(new Sequencer.MoveSelectedDataPoints(chart, left, right));
                    }
                    else // else if XYchart
                    {
                        moveSelectedDataPoints.Add(new MoveSelectedDataPoints(chart, left, right, up, down));
                    }
                }
            }
        }

        public bool Do()
        {
            if (moveSelectedDataPoints == null || moveSelectedDataPoints.Count == 0) return false;

            for (int i = 0; i < moveSelectedDataPoints.Count; ++i) // find the smallest valid change in x and y
            {
                if(!moveSelectedDataPoints[i].Try())
                {
                    return false;
                }

                if (!moveSelectedDataPoints[i].left)
                {
                    left = false;
                }

                if (!moveSelectedDataPoints[i].right)
                {
                    right = false;
                }

                if (!moveSelectedDataPoints[i].up)
                {
                    up = false;
                }

                if (!moveSelectedDataPoints[i].down)
                {
                    down = false;
                }

                if (!left && !right && !up && !down)
                {
                    return false;
                }
            }

            for (int i = 0; i < moveSelectedDataPoints.Count; ++i)
            {
                moveSelectedDataPoints[i].UpdateValues(left, right, up, down);
            }

            return true;
        }

        public void Undo()
        {
            foreach (MoveSelectedDataPoints moveSelectedDataPoint in moveSelectedDataPoints)
            {
                moveSelectedDataPoint.Undo();
            }
        }
    }
}
