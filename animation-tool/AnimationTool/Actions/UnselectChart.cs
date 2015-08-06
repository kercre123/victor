using System.Windows.Forms.DataVisualization.Charting;
using System.Drawing;

namespace AnimationTool
{
    public class UnselectChart : Action
    {
        public static readonly Color borderlineColor = Color.Transparent;

        protected Chart chart;
        protected UnselectAllDataPoints unselectAllDataPoints;

        public UnselectChart(Chart chart)
        {
            this.chart = chart;
            unselectAllDataPoints = new UnselectAllDataPoints(chart);
        }

        public virtual bool Do()
        {
            if (chart == null || unselectAllDataPoints == null || chart.BorderlineColor == borderlineColor) return false;

            if (!unselectAllDataPoints.Do())
            {
                unselectAllDataPoints = null;
            }

            chart.BorderlineColor = borderlineColor;

            chart.Refresh();

            return true;
        }

        public virtual void Undo()
        {
            if (chart == null || unselectAllDataPoints == null) return;

            chart.BorderlineColor = SelectChart.borderlineColor;

            if (unselectAllDataPoints != null)
            {
                unselectAllDataPoints.Undo();
            }

            chart.Refresh();
        }
    }
}
