using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class UnselectAllCharts : Action
    {
        protected List<UnselectChart> unselectCharts;

        public UnselectAllCharts(List<Component> components, Chart ignore = null)
        {
            unselectCharts = new List<UnselectChart>();

            foreach (Chart chart in components)
            {
                if (chart == ignore) continue;

                unselectCharts.Add(new UnselectChart(chart));
            }
        }

        public virtual bool Do()
        {
            if (unselectCharts == null) return false;

            for (int i = 0; i < unselectCharts.Count; ++i)
            {
                if (!unselectCharts[i].Do())
                {
                    unselectCharts.RemoveAt(i--);
                }
            }

            return unselectCharts.Count > 0;
        }

        public virtual void Undo()
        {
            foreach (UnselectChart action in unselectCharts)
            {
                action.Undo();
            }
        }
    }
}
