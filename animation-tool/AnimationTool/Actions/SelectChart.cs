using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool
{
    public class SelectChart : Action
    {
        public static readonly Color borderlineColor = Color.Red;

        protected Chart chart;
        protected UnselectAllCharts unselectAllCharts;

        public SelectChart(Chart chart, List<ChartForm> charts = null)
        {
            this.chart = chart;

            if (charts != null)
            {
                unselectAllCharts = new UnselectAllCharts(charts, chart);
            }
        }

        public virtual bool Do()
        {
            if (chart == null) return false;

            if (unselectAllCharts != null)
            {
                if (!unselectAllCharts.Do())
                {
                    unselectAllCharts = null;
                }
            }

            chart.BorderlineColor = borderlineColor;

            chart.Refresh();

            return true;
        }

        public virtual void Undo()
        {
            if (unselectAllCharts != null)
            {
                unselectAllCharts.Undo();
            }

            chart.BorderlineColor = UnselectChart.borderlineColor;

            chart.Refresh();
        }
    }
}
