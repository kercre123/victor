using System.Collections.Generic;
using System.Windows.Forms;
using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class EnableChart : Action
    {
        protected Chart chart;
        protected UnselectAllDataPoints unselectAllDataPoints;
        protected List<RemoveDataPoint> removeDataPoints;
        protected CheckBox checkBox;

        protected EnableChart() { }

        public EnableChart(ChartForm form)
        {
            chart = form.chart;
            checkBox = form.CheckBox;
            removeDataPoints = new List<RemoveDataPoint>();
            unselectAllDataPoints = new UnselectAllDataPoints(chart);
        }

        public virtual bool Do()
        {
            if (chart == null || chart.Enabled || checkBox == null) return false;

            chart.Enabled = true;

            if (checkBox.Checked)
            {
                checkBox.Checked = false;
            }

            // put first and last point in middle value
            double y = Math.Round((chart.ChartAreas[0].AxisY.Minimum + chart.ChartAreas[0].AxisY.Maximum) * 0.5, 0);

            if (chart.Series[0].Points.Count == 0)
            {
                AddDataPoint addFirst = new AddDataPoint(chart, chart.ChartAreas[0].AxisX.Minimum, y, false);

                addFirst.Do();
                addFirst.dataPoint.MarkerColor = System.Drawing.Color.Black;
            }

            if (chart.Series[0].Points.Count == 1)
            {
                AddDataPoint addLast = new AddDataPoint(chart, chart.ChartAreas[0].AxisX.Maximum, y, false);

                addLast.Do();
            }

            if (!unselectAllDataPoints.Do())
            {
                unselectAllDataPoints = null;
            }

            for (int i = 0; i < chart.Series[0].Points.Count; ++i)
            {
                chart.Series[0].Points[i].IsEmpty = false;
            }

            chart.Refresh();

            return true;
        }

        public virtual void Undo()
        {
            if (checkBox != null && !checkBox.Checked)
            {
                checkBox.Checked = true;
            }

            for (int i = 0; i < chart.Series[0].Points.Count; ++i)
            {
                chart.Series[0].Points[i].IsEmpty = true;
            }

            if (unselectAllDataPoints != null)
            {
                unselectAllDataPoints.Undo();
            }

            chart.Enabled = false;

            chart.Refresh();
        }
    }
}
