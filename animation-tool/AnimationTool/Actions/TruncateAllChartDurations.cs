using System.Collections.Generic;
using System.ComponentModel;
using System.Windows.Forms.DataVisualization.Charting;
using System;

namespace AnimationTool
{
    public class TruncateAllChartDurations : Action
    {
        protected List<TruncateChartDuration> truncateChartDurations;

        protected double xValueOld;
        protected double xValueNew;

        public TruncateAllChartDurations(List<Component> components, double xValueNew)
        {
            truncateChartDurations = new List<TruncateChartDuration>();

            this.xValueNew = Math.Round(xValueNew, 1);
            xValueOld = Math.Round(Properties.Settings.Default.maxTime, 1);

            foreach (Chart chart in components)
            {
                DataPoint dp = chart.Series[0].Points[0]; // if chart has sequencer point
                bool sequencer = dp.IsCustomPropertySet(Sequencer.ExtraData.Key) && 
                    Sequencer.ExtraData.Entries.ContainsKey(dp.GetCustomProperty(Sequencer.ExtraData.Key));

                if (sequencer)
                {
                    truncateChartDurations.Add(new Sequencer.TruncateChartDuration(chart, xValueNew));
                }
                else // else if XYchart
                {
                    truncateChartDurations.Add(new TruncateChartDuration(chart, xValueNew));
                }
            }
        }

        public virtual bool Do()
        {
            if (truncateChartDurations == null || xValueOld == xValueNew) return false;

            for (int i = 0; i < truncateChartDurations.Count; ++i)
            {
                if (!truncateChartDurations[i].Do())
                {
                    truncateChartDurations.RemoveAt(i--);
                }
            }

            if (truncateChartDurations.Count == 0)
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

            foreach (TruncateChartDuration action in truncateChartDurations)
            {
                action.Undo();
            }
        }
    }
}
