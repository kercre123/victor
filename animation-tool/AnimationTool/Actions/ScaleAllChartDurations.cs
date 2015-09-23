using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class ScaleAllChartDurations : Action
    {
        protected List<ScaleChartDuration> scaleChartDurations;

        protected double xValueOld;
        protected double xValueNew;

        public ScaleAllChartDurations(List<ChartForm> chartForms, double xValueNew)
        {
            scaleChartDurations = new List<ScaleChartDuration>();

            this.xValueNew = Math.Round(xValueNew, 1);
            xValueOld = Math.Round(Properties.Settings.Default.maxTime, 1);

            foreach (ChartForm chartForm in chartForms)
            {
                DataPoint dp = chartForm.chart.Series[0].Points[0]; // if chart has sequencer point
                bool sequencer = dp.IsCustomPropertySet(Sequencer.ExtraData.Key) && Sequencer.ExtraData.Entries.ContainsKey(dp.GetCustomProperty(Sequencer.ExtraData.Key));

                if (sequencer)
                {
                    scaleChartDurations.Add(new Sequencer.ScaleChartDuration(chartForm.chart, xValueNew));
                }
                else // else if XYchart
                {
                    scaleChartDurations.Add(new ScaleChartDuration(chartForm.chart, xValueNew));
                }
            }
        }

        public virtual bool Do()
        {
            if (scaleChartDurations == null || xValueNew == xValueOld) return false;

            for (int i = 0; i < scaleChartDurations.Count; ++i)
            {
                if(!scaleChartDurations[i].Do())
                {
                    scaleChartDurations.RemoveAt(i--);
                }
            }

            if (scaleChartDurations.Count == 0)
            {
                return false;
            }

            Properties.Settings.Default["maxTime"] = Math.Round(xValueNew, 1);
            Properties.Settings.Default.Save();

            return true;
        }

        public virtual void Undo()
        {
            Properties.Settings.Default["maxTime"] = Math.Round(xValueOld, 1);
            Properties.Settings.Default.Save();

            foreach (ScaleChartDuration action in scaleChartDurations)
            {
                action.Undo();
            }
        }
    }
}
