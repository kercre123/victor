using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class SelectAllDataPoints : Action
    {
        protected List<SelectDataPoint> actions;
        protected DataPointCollection points;

        public SelectAllDataPoints(Chart chart)
        {
            actions = new List<SelectDataPoint>();
            points = chart.Series[0].Points;
        }

        public virtual bool Do()
        {
            if (actions == null) return false;

            for(int i = 1; i < points.Count; ++i)
            {
                DataPoint dataPoint = points[i];

                if (dataPoint.IsEmpty || dataPoint.Color == Color.Red) continue;

                SelectDataPoint action = new SelectDataPoint(dataPoint);

                if(action.Do())
                {
                    actions.Add(action);
                }
            }

            return true;
        }

        public virtual void Undo()
        {
            if (actions == null) return;

            foreach(SelectDataPoint action in actions)
            {
                action.Undo();
            }
        }
    }
}
