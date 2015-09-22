using System.Collections.Generic;
using System.Drawing;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;

namespace AnimationTool
{
    public class DisableChart : Action
    {
        protected Chart chart;
        protected CheckBox checkBox;
        protected UnselectChart unselectChart;

        public DisableChart(ChartForm form)
        {
            chart = form.chart;
            checkBox = form.checkBox;
            unselectChart = new UnselectChart(chart);
        }

        public virtual bool Do()
        {
            if (chart == null || !chart.Enabled || checkBox == null) return false;

            if(!unselectChart.Do())
            {
                unselectChart = null;
            }

            if (!checkBox.Checked)
            {
                checkBox.Checked = true;
            }

            for (int i = 0; i < chart.Series[0].Points.Count; ++i)
            {
                chart.Series[0].Points[i].IsEmpty = true;
            }

            chart.Refresh();

            chart.Enabled = false;

            return true;
        }

        public virtual void Undo()
        {
            chart.Enabled = true;

            for (int i = 0; i < chart.Series[0].Points.Count; ++i)
            {
                chart.Series[0].Points[i].IsEmpty = false;
            }

            if (unselectChart != null)
            {
                unselectChart.Undo();
            }

            if (checkBox != null && !checkBox.Checked)
            {
                checkBox.Checked = false;
            }

            chart.Refresh();
        }
    }
}
